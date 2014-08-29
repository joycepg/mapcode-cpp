/**
 * Copyright (C) 2014 Stichting Mapcode Foundation
 * For terms of use refer to http://www.mapcode.com/downloads.html
 */

#include <stdio.h>
#include <math.h>
#include "mapcoder/mapcoder.c"

static const double PI = 3.14159265358979323846;
static const int RESULTS_MAX = 64;
static const int SHOW_PROGRESS = 250;

static void usage(const char* appName) {
    printf("MAPCODE (C library version %s)\n", mapcode_cversion);
    printf("Copyright (C) 2014 Stichting Mapcode Foundation\n");
    printf("\n");
    printf("Usage:\n");
    printf("    %s [-d | --decode] <default-territory> <mapcode> [<mapcode> ...]\n", appName);
    printf("\n");
    printf("       Decode a Mapcode to a lat/lon. The default territory code is used if\n");
    printf("       the Mapcode is a shorthand local code\n");
    printf("\n");
    printf("    %s [-e | --encode] <lat:-90..90> <lon:-180..180> [territory]>\n", appName);
    printf("\n");
    printf("       Encode a lat/lon to a Mapcode. If the territory code is specified, the\n");
    printf("       encoding will only succeeed if the lat/lon is located in the territory.\n");
    printf("\n");
    printf("    %s [-b | --boundaries]\n", appName);
    printf("    %s [-g | --grid] <nrPoints>\n", appName);
    printf("    %s [-r | --random] <nrPoints> [<seed>]\n", appName);
    printf("\n");
    printf("       Create a test set of lat/lon pairs based on the Mapcode boundaries database\n");
    printf("       as a fixed 3D grid or random uniformly distributed set of lat/lons with their\n");
    printf("       (x, y, z) coordinates and all Mapcode aliases.\n");
    printf("\n");
    printf("       The output format is:\n");
    printf("           <number-of-aliases> <lat-deg> <lon-deg> <x> <y> <z>\n");
    printf("           <territory> <mapcode>      (repeated 'number-of-aliases' times)\n");
    printf("                                      (empty lines and next record)\n");
    printf("       Ranges:\n");
    printf("           number-of-aliases : >= 1\n");
    printf("           lat-deg, lon-deg  : [-90..90], [-180..180]\n");
    printf("           x, y, z           : [-1..1]\n");
    printf("\n");
    printf("\n     stdout: used for outputting 3D point data; stderr: used for statistics.");
    printf("\n");
    printf("       The lat/lon pairs will be distributed over the 3D surface of the Earth\n");
    printf("       and the (x, y, z) coordinates are placed on a sphere with radius 1.\n");
}

static double radToDeg(double rad){
    return (rad / PI) * 180.0;
}

static double degToRad(double deg){
    return (deg / 180.0) * PI;
}

/**
 * Given a single number between 0..1, generate a latitude, longitude (in degrees) and a 3D
 * (x, y, z) point on a sphere with a radius of 1.
 */
static void unitToLatLonDeg(
    const double unit1, const double unit2, double* latDeg, double* lonDeg) {

    // Calculate uniformly distributed 3D point on sphere (radius = 1.0):
    // http://mathproofs.blogspot.co.il/2005/04/uniform-random-distribution-on-sphere.html
    const double theta0 = (2.0 * PI) * unit1;
    const double theta1 = acos(1.0 - (2.0 * unit2));
    double x = sin(theta0) * sin(theta1);
    double y = cos(theta0) * sin(theta1);
    double z = cos(theta1);

    // Convert Carthesian 3D point into lat/lon (radius = 1.0):
    // http://stackoverflow.com/questions/1185408/converting-from-longitude-latitude-to-cartesian-coordinates
    const double latRad = asin(z);
    const double lonRad = atan2(y, x);

    // Convert radians to degrees.
    *latDeg = isnan(latRad) ? 90.0 : radToDeg(latRad);
    *lonDeg = isnan(lonRad) ? 180.0 : radToDeg(lonRad);
}

static void convertLatLonToXYZ(double latDeg, double lonDeg, double* x, double* y, double* z) {
    double latRad = degToRad(latDeg);
    double lonRad = degToRad(lonDeg);
    *x = cos(latRad) * cos(lonRad);
    *y = cos(latRad) * sin(lonRad);
    *z = sin(latRad);
}

static int printMapcodes(double lat, double lon, int iShowError) {
    const char* results[RESULTS_MAX];
    int context = 0;
    const int nrResults = coord2mc(results, lat, lon, context);
    if (nrResults <= 0) {
        if (iShowError) {
            fprintf(stderr, "error: cannot encode lat=%f, lon=%f)\n", lat, lon);
        }
        return -1;
    }
    double x;
    double y;
    double z;
    convertLatLonToXYZ(lat, lon, &x, &y, &z);
    printf("%d %lf %lf %lf %lf %lf\n", nrResults, lat, lon, x, y, z);
    for (int j = 0; j < nrResults; ++j) {
        printf("%s %s\n", results[(j * 2) + 1], results[(j * 2)]);
    }
    printf("\n");
    return nrResults;
}

int main(const int argc, const char** argv)
{
    // Provide usage message if no arguments specified.
    const char* appName = argv[0];
    if (argc < 2) {
        usage(appName);
        return -1;
    }

    // First argument: command.
    const char* cmd = argv[1];
    if ((strcmp(cmd, "-d") == 0) || (strcmp(cmd, "--decode") == 0)) {

        // ------------------------------------------------------------------
        // Decode: [-d | --decode] <default-territory> <mapcode> [<mapcode> ...]
        // ------------------------------------------------------------------
        if (argc < 4) {
            fprintf(stderr, "error: incorrect number of arguments\n\n");
            usage(appName);
            return -1;
        }

        const char* defaultTerritory = argv[2];
        double lat;
        double lon;
        int context = text2tc(defaultTerritory, 0);
        for (int i = 3; i < argc; ++i) {
            int err = mc2coord(&lat, &lon, argv[i], context);
            if (err != 0) {
                fprintf(stderr, "error: cannot decode '%s' (default territory='%s')\n", argv[i], argv[2]);
                return -1;
            }
            printf("%f %f\n", lat, lon);
        }
    }
    else if ((strcmp(cmd, "-e") == 0) || (strcmp(cmd, "--encode") == 0)) {

        // ------------------------------------------------------------------
        // Encode: [-e | --encode] <lat:-90..90> <lon:-180..180> [territory]>
        // ------------------------------------------------------------------
        if ((argc != 4) && (argc != 5)) {
            fprintf(stderr, "error: incorrect number of arguments\n\n");
            usage(appName);
            return -1;
        }
        const double lat = atof(argv[2]);
        const double lon = atof(argv[3]);

        int context = 0;
        if (argc == 5) {
            context = text2tc(argv[4], 0);
        }
        const char* results[RESULTS_MAX];
        const int nrResults = coord2mc(results, lat, lon, context);
        if (nrResults <= 0) {
            fprintf(stderr, "error: cannot encode lat=%s, lon=%s (default territory=%d)\n",
                argv[2], argv[3], context);
            return -1;
        }
        for (int i = 0; i < nrResults; ++i) {
            printf("%s %s\n", results[(i * 2) + 1], results[(i * 2)]);
        }
    }
    else if ((strcmp(cmd, "-b") == 0) || (strcmp(cmd, "--boundaries") == 0)) {

        // ------------------------------------------------------------------
        // Generate a test set based on the Mapcode boundaries.
        // ------------------------------------------------------------------
        if (argc != 2) {
            fprintf(stderr, "error: incorrect number of arguments\n\n");
            usage(appName);
            return -1;
        }
        for (int i = 0; i < NR_RECS; ++i) {
            long minLonE6;
            long maxLonE6;
            long minLatE6;
            long maxLatE6;
            double minLon;
            double maxLon;
            double minLat;
            double maxLat;
            double lat;
            double lon;

            get_boundaries(i, &minLonE6, &minLatE6, &maxLonE6, &maxLatE6);
            minLon = ((double) minLonE6) / 1.0E6;
            maxLon = ((double) maxLonE6) / 1.0E6;
            minLat = ((double) minLatE6) / 1.0E6;
            maxLat = ((double) maxLatE6) / 1.0E6;

            // Try center.
            lat = (maxLat - minLat ) / 2.0;
            lon = (maxLon - minLon ) / 2.0;

            // Try center.
            printMapcodes(lat, lon, 0);

            // Try corners.
            printMapcodes(minLat, minLon, 0);
            printMapcodes(minLat, maxLon, 0);
            printMapcodes(maxLat, minLon, 0);
            printMapcodes(maxLat, maxLon, 0);

            // Try JUST inside.
            double factor = 1.0;
            for (int j = 1; j < 6; ++j) {

                double d = 1.0 / factor;
                printMapcodes(minLat + d, minLon + d, 0);
                printMapcodes(minLat + d, maxLon - d, 0);
                printMapcodes(maxLat - d, minLon + d, 0);
                printMapcodes(maxLat - d, maxLon - d, 0);

                // Try JUST outside.
                printMapcodes(minLat - d, minLon - d, 0);
                printMapcodes(minLat - d, maxLon + d, 0);
                printMapcodes(maxLat + d, minLon - d, 0);
                printMapcodes(maxLat + d, maxLon + d, 0);
                factor = factor * 10.0;
            }

            // Try 22m outside.
            printMapcodes(minLat - 22, (maxLon - minLon) / 2, 0);
            printMapcodes(minLat - 22, (maxLon - minLon) / 2, 0);
            printMapcodes(maxLat + 22, (maxLon - minLon) / 2, 0);
            printMapcodes(maxLat + 22, (maxLon - minLon) / 2, 0);

            if ((i % SHOW_PROGRESS) == 0) {
                fprintf(stderr, "Processed %d of %d regions...\r", i, NR_RECS);
            }
        }
    }
    else if ((strcmp(cmd, "-g") == 0) || (strcmp(cmd, "--grid") == 0) ||
        (strcmp(cmd, "-r") == 0) || (strcmp(cmd, "--random") == 0)) {

        // ------------------------------------------------------------------
        // Generate grid test set:    [-g | --grid]   <nrPoints>
        // Generate uniform test set: [-r | --random] <nrPoints> [<seed>]
        // ------------------------------------------------------------------
        if ((argc < 3) || (argc > 4)) {
            fprintf(stderr, "error: incorrect number of arguments\n\n");
            usage(appName);
            return -1;
        }
        const int nrPoints = atoi(argv[2]);
        if (nrPoints < 1) {
            fprintf(stderr, "error: total number of points to generate must be >= 1\n\n");
            usage(appName);
            return -1;
        }
        int random = (strcmp(cmd, "-r") == 0) || (strcmp(cmd, "--random") == 0);
        if (random) {
            if (argc == 4) {
                const int seed = atoi(argv[3]);
                srand(seed);
            }
            else {
                srand(time(0));
            }
        }
        else {
            if (argc > 3) {
                fprintf(stderr, "error: cannot specify seed for -g/--grid\n\n");
                usage(appName);
                return -1;
            }
        }

        // Statistics.
        int largestNrOfResults = 0;
        double latLargestNrOfResults = 0.0;
        double lonLargestNrOfResults = 0.0;
        int totalNrOfResults = 0;

        int gridX = 0;
        int gridY = 0;
        int line = round(sqrt(nrPoints));
        for (int i = 0; i < nrPoints; ++i) {
            double lat;
            double lon;
            double unit1;
            double unit2;

            if (random) {
                unit1 = ((double) rand()) / RAND_MAX;
                unit2 = ((double) rand()) / RAND_MAX;
            }
            else {
                unit1 = ((double) gridX) / line;
                unit2 = ((double) gridY) / line;

                if (gridX < line) {
                    ++gridX;
                }
                else {
                    gridX = 0;
                    ++gridY;
                }
            }

            unitToLatLonDeg(unit1, unit2, &lat, &lon);
            const int nrResults = printMapcodes(lat, lon, 1);
            if (nrResults > largestNrOfResults) {
                largestNrOfResults = nrResults;
                latLargestNrOfResults = lat;
                lonLargestNrOfResults = lon;
            }
            totalNrOfResults += nrResults;
            if ((i % SHOW_PROGRESS) == 0) {
                fprintf(stderr, "Created %d of %d 3D %s data points...\r", i, nrPoints, random ? "random" : "grid");
            }
        }
        fprintf(stderr, "\nStatistics:\n");
        fprintf(stderr, "Total number of 3D points generated     = %d\n", nrPoints);
        fprintf(stderr, "Total number of Mapcodes generated      = %d\n", totalNrOfResults);
        fprintf(stderr, "Average number of Mapcodes per 3D point = %f\n", ((float) totalNrOfResults) / ((float) nrPoints));
        fprintf(stderr, "Largest number of results for 1 Mapcode = %d at (%f, %f)\n", largestNrOfResults, latLargestNrOfResults, lonLargestNrOfResults);
    }
    else {

        // ------------------------------------------------------------------
        // Usage.
        // ------------------------------------------------------------------
        usage(appName);
        return -1;
    }
    return 0;
}
