#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include "location.h"
#include "track.h"

int main(int argc, char *argv[])
{
    //time bound, distance bound, then name of file
    FILE *in;
    char *ptr1;
    char *ptr2;
    double timebound = strtod(argv[1], &ptr1);
    double distbound = strtod(argv[2], &ptr2);
    if (timebound <= 0 || distbound < 0 || argv[1] == ptr1 || argv[2] == ptr2){
        return 1;
    }
    if (argc == 3)
    {
        // no filename given, read from standard input instead
        in = stdin;
    }
    else if(argc == 4)
    {
        in = fopen(argv[3], "r"); // mode string "w" for writing (with replacement); "w+" for append
    }
    else
        return 1;

    if (in == NULL)
    {
        fprintf(stderr, "Stationary: error opening %s\n", argv[3]);
        return 1;
    }
    track* inputtrack = track_create();
    trackpoint* newpt = (trackpoint*)malloc(sizeof(trackpoint));
    if (newpt != NULL){
        while (fscanf(in, "%lf %lf %lf", &(newpt->loc.lat), &(newpt->loc.lon), &(newpt->time)) != EOF){
            if(track_add_point(inputtrack,newpt) == false){
                return 1;
            }
        }
    }
    else
         return 1;
    int n = 0;
    trackpoint* nearstationary = track_find_stationary(inputtrack,timebound,distbound,&n);
    track_destroy(inputtrack);
    free(newpt);
    if (nearstationary != NULL){
        for (int i = 0; i < n; i++){
            printf("%lf %lf\n", nearstationary[i].loc.lat, nearstationary[i].loc.lon);
        }
    }
    else
        return 1;
    free(nearstationary);
    fclose(in);
    return 0;
}
