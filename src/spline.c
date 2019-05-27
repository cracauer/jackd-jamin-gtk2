/*
    This source is in the public domain.  It is distributed in the hope that 
    it will be useful, but WITHOUT ANY WARRANTY; without even the implied 
    warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
*/

#include <stdio.h>
#include <stdlib.h>
#include <math.h>

#define SPLINE_COL  53
#define SPLINE_ROW  3


/***************************************************************************\
*                                                                           *
*   Programmer(s):      Dominic Avery, Jan C. Depner                        *
*                                                                           *
*   Date Written:       April 1992                                          *
*                                                                           *
*   Module Name:        spline_cof                                          *
*                                                                           *
*   Purpose:            Computes coefficients for use in the spline         *
*                       function.                                           *
*                                                                           *
*   Inputs:             x               -   pointer to x data               *
*                       y               -   pointer to y data               *
*                       pos             -   position in array               *
*                       coeffs          -   coefficient array               *
*                                                                           *
*   Outputs:            None                                                *
*                                                                           *
*   Calling Routines:   spline                                              *
*                                                                           * 
*   Glossary:           index           -   utility integer                 *
*                       posval          -   pos - 1                         *
*                       inv_index       -   inverse index                   *
*                       array_a         -                                   *
*                       array_d         -                                   *
*                       array_p         -                                   *
*                       array_e         -                                   *
*                       array_b         -                                   *
*                       array_z         -                                   *
*                       value           -   utility float                   *
*                                                                           * 
\***************************************************************************/

void spline_cof (float *x, float *y, int pos, float *coeffs)
{
    int        index, posval, inv_index;
    float      array_a[SPLINE_COL][SPLINE_ROW], array_d[SPLINE_COL], 
               array_p[SPLINE_COL], array_e[SPLINE_COL], 
               array_b[SPLINE_COL], array_z[SPLINE_COL], value;

    for (index = 0; index < pos; index++)
    {
        array_d[index] = *(x + (index + 1)) - *(x + index);
        array_p[index] = array_d[index] / 6.0;
        array_e[index] = (*(y + (index + 1)) - *(y + index)) / array_d[index];
    }

    for (index = 1; index < pos; index++)
        array_b[index] = array_e[index] - array_e[index - 1];

    array_a[0][1] = -1.0 - array_d[0] / array_d[1];
    array_a[0][2] = array_d[0] / array_d[1];
    array_a[1][2] = array_p[1] - array_p[0] * array_a[0][2];
    array_a[1][1] = 2.0 * (array_p[0] + array_p[1]) - array_p[0] *
        array_a[0][1];
    array_a[1][2] = array_a[1][2] / array_a[1][1];
    array_b[1] = array_b[1] / array_a[1][1];

    for (index = 2; index < pos; index++)
    {
        array_a[index][1] = 2.0 * (array_p[index - 1] + array_p[index]) - 
            array_p[index - 1] * array_a[index - 1][2];
        array_b[index] = array_b[index] - array_p[index - 1] *
            array_b[index - 1];
        array_a[index][2] = array_p[index] / array_a[index][1];
        array_b[index] = array_b[index] / array_a[index][1];
    }

    value =  array_d[pos - 2] / array_d[pos - 2];
    array_a[pos][0] = 1.0 + value + array_a[pos - 2][2];
    array_a[pos][1] = -value - array_a[pos][0] * array_a[pos - 1][2];
    array_b[pos] = array_b[pos - 2] - array_a[pos][0] * array_b[pos - 1];
    array_z[pos] = array_b[pos] / array_a[pos][1];
    posval = pos - 1;

    for (index = 0; index < posval; index++)
    {
        inv_index = pos - index - 1;
        array_z[inv_index] = array_b[inv_index] - array_a[inv_index][2] * 
            array_z[inv_index + 1];
    }

    array_z[0] = -array_a[0][1] * array_z[1] - array_a[0][2] * array_z[2];

    for (index = 0; index < pos; index++)
    {
        value = 1.0 / (6.0 * array_d[index]);
        *(coeffs + index) = array_z[index] * value;
        *(coeffs + SPLINE_COL + index) = array_z[index + 1] * value;
        *(coeffs + (2 * SPLINE_COL) + index) = *(y + index) / array_d[index] - 
            array_z[index] * array_p[index];
        *(coeffs + (3 * SPLINE_COL) + index) = *(y + index + 1) / 
            array_d[index] - array_z[index + 1] * array_p[index];
    }

    return;
}



/***************************************************************************\
*                                                                           *
*   Programmer(s):      Dominic Avery, Jan C. Depner                        *
*                                                                           *
*   Date Written:       April 1992                                          *
*                                                                           *
*   Module Name:        spline                                              *
*                                                                           *
*   Purpose:            Computes 53 point bicubic spline interpolated data  *
*                       from input data.                                    *
*                                                                           *
*   Inputs:             x               -   input x data                    *
*                       y               -   input y data                    *
*                       pos             -   position in array               *
*                       x_pos           -   x value                         *
*                       y_pos           -   y value computed                *
*                       ater            -                                   *
*                                                                           *
*   Outputs:            None                                                *
*                                                                           *
*   Calling Routines:   interpolate                                         *
*                                                                           * 
*   Glossary:           endloop1        -   end of loop flag                *
*                       endloop2        -   end of loop flag                *
*                       coeffs          -   coefficient array               *
*                       valpos          -                                   *
*                                                                           *
*   Method:             bicubic spline                                      *
*                                                                           * 
\***************************************************************************/

void spline (float *x, float *y, int pos, float x_pos, float *y_pos, float *ater)
{
    int            endloop1, endloop2;
    float          coeffs[SPLINE_ROW + 1][SPLINE_COL];
    static int     valpos = 0;

    if ((*x + *(y + pos) + *(y + (pos - 1)) + *(x + (pos - 1)) +
        *(y + (pos - 2)) - *ater) != 0.0)
    {
        spline_cof (x, y, pos, &coeffs[0][0]);

        *ater = *x + *(y + pos) + *(y + (pos - 1)) + *(x + (pos - 1)) + 
                *(y + (pos - 2));
        valpos = 0;
    }

    endloop1 = 0;
    endloop2 = 0;

    while (!endloop1)
    {
        endloop1 = 1;
        if (fabs ((double) (x_pos - *x)) >= 0.00001)
        {
            if ((x_pos - *x) < 0) 
            {
                valpos = 0;
/*                printf
                    ("Caution value at position %f was linearly extrapolated\n",
                    x_pos);*/
               
                /*  Extrapolate with average slope of closest two       */
                /*  intervals.                                          */

                *y_pos = *y + (x_pos - *x) * ((*(y + 1) - *y) /
                    (*(x + 1) - *x) + (*(y + 2) - *(y + 1)) / (*(x + 2) -
                    *(x + 1))) * 0.5;

                return;
            }
            else if ((x_pos - *x) == 0) 
            {
                *y_pos = *y;
                return;
            }
            else
            {
                if ((fabs ((double) (x_pos - *(x + (valpos + 1))))) >= 
                    0.00001)
                {
                    if ((x_pos - *(x + (valpos + 1))) < 0)
                    {
                        while (!endloop2)
                        {
                            endloop2 = 1;
                            if ((fabs ((double) (x_pos - 
                                *(x + (valpos + 1))))) >= 0.00001)
                            {
                                if (fabs ((double) (x_pos - 
                                    *(x + valpos))) < 0)
                                {
                                    valpos--;
                                    endloop2 = 0;
                                }
                                else if (fabs ((double) (x_pos - 
                                    *(x + valpos))) == 0)
                                {
                                    *y_pos = *(y + valpos);
                                    return;
                                }
                                else
                                {
                                    *y_pos = (*(x + (valpos + 1)) - x_pos) *
                                        (coeffs[0][valpos] *
                                        (*(x + (valpos + 1)) - x_pos) *
                                        (*(x + (valpos + 1)) - x_pos) + 
                                        coeffs[2][valpos]);

                                    *y_pos += (x_pos - *(x + valpos)) *
                                        (coeffs[1][valpos] * 
                                        (x_pos - *(x + valpos)) *
                                        (x_pos - *(x + valpos)) +
                                        coeffs[3][valpos]);
                                    return;
                                }
                            }
                            else
                            {
                                *y_pos = *(y + valpos);
                                return;
                            }
                        }
                    }
                    else if ((x_pos - *(x + (valpos + 1))) == 0)
                    {
                        *y_pos = *(y + (valpos + 1));
                        return;
                    }
                    
                    else
                    {
                        valpos++;
                        if ((pos - valpos) > 0)
                        {
                            endloop1 = 0;
                        }
                        else
                        {
                            valpos = pos - 1;
    
/*                            printf
                                ("Caution value at position %f was linearly extrapolated\n",
                                x_pos);*/
                    
                            /*  Extrapolate with average slope of       */
                            /*  closest two intervals.                  */
    
                            if (valpos == 1)
                            {
                                *y_pos = *y + (x_pos - *x) * ((*(y + 1) - *y) /
                                    (*(x + 1) - *x) + (*(y + 2) - *(y + 1)) /
                                    (*(x + 2) - *(x + 1))) * 0.5;
                            }
                            else
                            {
                                *y_pos = *(y + pos) + (x_pos - *(x + pos)) *
                                    ((*(y + valpos) - *(y + valpos - 1)) / 
                                    (*(x + valpos) - *(x + (valpos - 1))) +
                                    (*(y + pos) - *(y + valpos)) /
                                    (*(x + pos) - *(x + valpos))) * 0.5;
                            }
                            return;
                        }
                    }
                }
                else
                {
                    *y_pos = *(y + (valpos + 1));
                    return;
                }
            }
        }
        else
        {
            *y_pos = *y;
            return;
        }
    }
}



/***************************************************************************\
*                                                                           *
*   Programmer(s):      Dominic Avery, Jan C. Depner                        *
*                                                                           *
*   Date Written:       April 1992                                          *
*                                                                           *
*   Module Name:        interpolate                                         *
*                                                                           *
*   Purpose:            Performs bicubic spline interpolation for an input  *
*                       data array.                                         *
*                                                                           *
*   Inputs:             interval        -   desired interpolation interval  *
*                       length_x        -   length of the input x array     *
*                       start_xinterp   -   starting position in x of the   *
*                                           output array                    *
*                       end_xinterp     -   ending position in x of the     *
*                                           output array                    *
*                       length_xinterp  -   length of the output array      *
*                       x               -   input x data                    *
*                       y               -   input y data                    *
*                       x_interp        -   interpolated x output data      *
*                       y_interp        -   interpolated y output data      *
*                                                                           *
*   Outputs:            None                                                *
*                                                                           *
*   Calling Routines:   misps                                               *
*                                                                           * 
*   Glossary:           num_segments    -   number of 50 point segments     *
*                       count           -   counter                         *
*                       length2         -   length of x in intervals        *
*                       endloop         -   end of loop flag                *
*                       index           -   utility integer                 *
*                       seg_points      -   number of points in the 50      *
*                                           point segments                  *
*                       flag1           -   utility flag                    *
*                       flag2           -   utility flag                    *
*                       last_x          -   end of segment                  *
*                       first_x         -   beginning of segment            *
*                       spline_length   -   length of data sent to spline   *
*                       current_x       -   position of data sent to spline *
*                       new_last_x      -   last_x for next segment         *
*                       new_first_x     -   first_x for next segment        *
*                       ater            -                                   *
*                       x_pos           -   x value for spline              *
*                       y_pos           -   y value from spline             *
*                                                                           *
*   Method:             Breaks up input data into 50 point pieces prior     *
*                       to sending it to the bicubic spline function.       *
*                                                                           * 
\***************************************************************************/

void interpolate (float interval, int length_x, float start_xinterp, float end_xinterp, 
                  int *length_xinterp, float *x, float *y, float *x_interp, 
                  float *y_interp)
{
    int   num_segments, count, length2, endloop, index, seg_points, flag1,
          flag2, last_x, first_x, spline_length, current_x, new_last_x,
          new_first_x;
            
    float ater, x_pos, y_pos;   

    void spline (float *, float *, int, float, float *, float *);


    /*  Compute length of interpolated data.                            */

    *length_xinterp =
        fabs ((double) ((end_xinterp - start_xinterp) / interval)) + 1.0;

    /*  Compute number of segments.                                     */
    
    num_segments = length_x / 50;
    seg_points = num_segments * 50;
    flag1 = 0;
    flag2 = 0;
    last_x = 48;
    if (seg_points == 0) last_x = length_x - 1;
    if ((length_x - seg_points) >= 2) flag2 = 1;

    /*  Overlap interpolation intervals by 2 input points so spline     */
    /*  routine is dimensioned to max of 53 points.                     */
    
    count = 1;
    first_x = 0;
    spline_length = last_x + 1;
    if (seg_points == 0) spline_length--;
    current_x = 0;
    endloop = 0;

    while (!endloop)
    {
        endloop = 1;

        /*  Break up the data into segments.                            */
        
        length2 =
            fabs ((double) ((*(x + last_x) - start_xinterp) / interval)) + 
            1.0;
        if (last_x == (length_x - 1)) length2 = *length_xinterp;
        ater = 9999.999;

        /*  Figure the length of the segment and whether it is the last */
        /*  or not.                                                     */

        for (index = first_x; index <= length2 - 1; index++)
        {
            x_pos = (float) index * interval + start_xinterp;

            if ((x_pos > *(x + last_x)) && (last_x != (length_x - 1)))
            {
                length2 = 0;
                count++;
                if (seg_points != 0)
                {
                    if (count <= num_segments)
                    {
                        new_last_x = count * 50 - 1;
                        new_first_x = last_x - 1;
                        current_x = new_first_x;
                        last_x += 50;
                        if (flag1) last_x = length_x - 1;
                        first_x = length2;
                        spline_length = new_last_x - new_first_x;
                        endloop = 0;
                        break;
                    }
                    if (flag2)
                    {
                        flag1 = 1;
                        flag2 = 0;
                        new_last_x = length_x - 1;
                        new_first_x = last_x - 1;
                        current_x = new_first_x;
                        last_x += 50;
                        if (flag1) last_x = length_x - 1;
                        first_x = length2;
                        spline_length = new_last_x - new_first_x;
                        endloop = 0;
                        break;
                    }
                }
                *length_xinterp = length2;
                return;
            }
            if (x_pos > end_xinterp)
            {
                *length_xinterp = index - 1;
                return;
            }

            /*  Spline it.                                              */

            spline ((x + current_x), (y + current_x), spline_length, x_pos,
                &y_pos, &ater);

            *(x_interp + index) = x_pos;
            *(y_interp + index) = y_pos;
        }

        if (endloop)
        {

            count++;
            if (seg_points != 0)
            {
                if (count <= num_segments)
                {
                    new_last_x = count * 50 - 1;
                    new_first_x = last_x - 1;
                    current_x = new_first_x;
                    last_x += 50;
                    if (flag1) last_x = length_x - 1;
                    first_x = length2;
                    spline_length = new_last_x - new_first_x;
                    endloop = 0;
                }
                else if (flag2)
                {
                    flag1 = 1;
                    flag2 = 0;
                    new_last_x = length_x - 1;
                    new_first_x = last_x - 1;
                    current_x = new_first_x;
                    last_x += 50;
                    if (flag1) last_x = length_x - 1;
                    first_x = length2;
                    spline_length = new_last_x - new_first_x;
                    endloop = 0;
                }
            }
        }
    }
    *length_xinterp = length2;

    return;
}
