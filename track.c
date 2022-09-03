#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include "location.h"
#include "track.h"

struct track{
    trackpoint* trackarray;
    int size;
};

/**
 * Creates an empty GPS track.
 *
 * @return a pointer to the track, or NULL if allocation was unsuccessful
 */
track *track_create(){
    track* newtrack = (track*)malloc(sizeof(track));
    if (newtrack != NULL){
        newtrack->trackarray = (trackpoint*)malloc(sizeof(trackpoint));
        newtrack->size = 0;
        return newtrack;
    }
    else
        return NULL;

}

/**
 * Destroys the given track.  The track is invalid after it has been destroyed.
 *
 * @param l a pointer to a track, non-NULL
 */
void track_destroy(track *l){
    if (l != NULL){
        free(l->trackarray);
        free(l);
    }
}

/**
 * Returns the number of trackpoints on the given track.
 *
 * @param t a pointer to a valid track, non-NULL
 * @return the number of points in t
 */
int track_size(const track *t){
    return t->size;
}

/**
 * Add a copy of the given trackpoint to the end of the given track.
 * The given trackpoint is added only if its time is strictly after the
 * time of the last trackpoint in t.
 *
 * @param t a pointer to a valid track, non-NULL
 * @param p a pointer to a valid trackpoint, non-NULL
 * @return true if and only if the point was successfully added
 */
bool track_add_point(track *t, const trackpoint *p){
    if(t != NULL){
        if(t->size == 0){
            t->trackarray[t->size] = *p;
            t->size++;
            return true;
        }
    }
    else
        return false;
    static int multiplier = 1;
    if(t->trackarray[t->size-1].time < p->time){
        if(t->size >= multiplier){
            t->trackarray= (trackpoint*)realloc(t->trackarray, sizeof(trackpoint)*multiplier*2);
            multiplier = multiplier*2;
        }
        if(t->trackarray != NULL){
            t->trackarray[t->size] = *p;
            t->size++;
            return true;
        }
        else
            return false;
    }
    else
        return false;
}

/**
 * Returns the time between the first trackpoint and the last trackpoint in
 * the given track, in seconds.  Returns zero if the track is empty.
 *
 * @param t a pointer to a valid track, non-NULL
 * @return the time between the first ans last trackpoints on that track
 */
double track_length(const track *t){
    if (t != NULL && t->size > 0){
        double timedif = t->trackarray[t->size-1].time - t->trackarray[0].time;
        return timedif;
    }
    else
        return 0.0;
}

/**
 * Returns a copy of the trackpoint on this track with the given time.
 * If there is no such trackpoint then the returned trackpoint is
 * determined by linear interpolation between the latitudes and longitudes
 * (separately) of the first trackpoints before
 * and after the given time.  If the given time is outside of the range
 * of times in the given track then the returned value is NULL and if
 * there is an error allocating the point to return then the returned
 * value is NULL.  Otherwise, it is the caller's responsibility to free the
 * returned trackpoint.
 *
 * @param t a pointer to a valid track, non-NULL
 * @param time a time
 * @return a copy of the trackpoint with the given time, or a new interpolated
 * trackpoint, or NULL
 */
//Note: log(n) is a hint that we should use binary search
trackpoint *track_get_point(const track *t, double time){
    trackpoint* newpt = (trackpoint*)malloc(sizeof(trackpoint));
    if (newpt != NULL && t->size > 0){
        int l = 0;
        int r = t->size-1;
        while (l <= r)
        {
            int m = l + (r-l)/2;
            if (t->trackarray[m].time == time){
                *newpt = t->trackarray[m];
                return newpt;
            }
            if (t->trackarray[m].time < time)
                l = m + 1;
            else
                r = m - 1;
        }
        if (t->trackarray[r].time < time && t->trackarray[l].time > time){
            double perctimedif = (time - t->trackarray[r].time)/(t->trackarray[l].time - t->trackarray[r].time);
            newpt->loc.lon = perctimedif*(t->trackarray[l].loc.lon - t->trackarray[r].loc.lon) + t->trackarray[r].loc.lon;
            newpt->loc.lat = perctimedif*(t->trackarray[l].loc.lat - t->trackarray[r].loc.lat) + t->trackarray[r].loc.lat;
            newpt->time = time;
            return newpt;
        }
        else
            return NULL;

    }
    else
        return NULL;
}

/**
 * Passes each point in the given track to the given function.
 *
 * @param t a pointer to a valid track, non-NULL
 * @param f the function to call for each point
 * @param arg an extra argument to pass to the function
 */
void track_for_each(const track *t, void (*f)(const trackpoint *, void *), void *arg){
    for (int i = 0; i < t->size; i++){
        f(&(t->trackarray[i]), arg);
    }
}

/**
 * Returns an array containing the trackpoints on the given track
 * where the track is near-stationary. If there is an error allocating space for
 * near-stationary points then the returned value is NULL and n is set
 * to 0.
 * A near-stationary point is defined as a trackpoint P that is
 * (1) at least as far from the end of the track as the given time bound
 * (2) the start of a part of the track for which all the following points strictly less than the time bound away
 * from P are no further than the distance bound away from P
 * (3) having at least one point X between P and the previous near-stationary point Q that is further away from Q
 * than the given distance bound (X may be P itself).
 *
 * @param t a pointer to a valid track, non-NULL
 * @param time the time threshold for determining near-stationary points, positive
 * @param dist the distance threshold for determining near-stationary points, positive
 * @param n a pointer to an integer in which to return the number
 * of near-stationary points
 * @return an array containing the near-stationary points,
 * in ascending order by time
 */
trackpoint *track_find_stationary(const track* t, double time, double dist, int *n){
    trackpoint* nearstat = (trackpoint*)malloc(sizeof(trackpoint));
    if (nearstat == NULL){
        *n = 0;
        return NULL;
    }
    int i = 0;
    int newind = 0;
    int multiplier = 1;
    while (i < t->size){
        if ((t->trackarray[t->size-1].time - t->trackarray[i].time) >= time){      //passes requirement #1
            int k = 0;
            int j = 0;
            while((t->trackarray[k+i].time - t->trackarray[i].time) < time){
                if (location_distance(&t->trackarray[j+i].loc, &t->trackarray[i].loc) <= dist){
                    j++;
                }
                k++;
            }
            if (j == k){ //passes requirement #2
                if ((newind+1) >= multiplier){
                    nearstat = (trackpoint*)realloc(nearstat, sizeof(trackpoint)*multiplier*2);
                    multiplier = multiplier*2;
                }
                if (nearstat == NULL){
                    *n = 0;
                    return NULL;
                }
                nearstat[newind] = t->trackarray[i];
                int l=0;
                while(l+i < t->size && location_distance(&t->trackarray[l+i].loc, &t->trackarray[i].loc) <= dist){
                    l++;
                }
                i += l;
                newind++;
            }
            else
                i++;

        }
        else
            break;
    }
    *n = newind;
    return nearstat;

}
