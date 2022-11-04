#include <math.h>

int invert_matrix( float m[100][100] , float res[100][100] , int n   )
{
    int rank = 0;
    unsigned int r,c,i;
    float temp;

    // initialize res to identity
    for( r = 0; r < n; r++ )
        for( c = 0; c < n; c++ )
        {
            if( r == c )
                res[r][c] = 1.0f;
            else
                res[r][c] = 0.0f;
        }

    for( i = 0; i < n; i++)
    {
        if( m[i][i] == 0.0f )
        {
            for( r = i; r < n; r++ )
                for( c = 0; c < n; c++ )
                {
                    m[i][c] += m[r][c];
                    res[i][c] += res[r][c];
                }
        }

        for( r = i; r < n; r++ )
        {
            temp = m[r][i];
            if( temp != 0.0f )
                for( c = 0; c < n; c++ )
                {
                    m[r][c] /= temp;
                    res[r][c] /= temp;
                }
        }

        if( i != n - 1 )
        {
            for( r = i + 1; r < n; r++ )
            {
                temp = m[r][i];
                if( temp != 0.0f )
                    for( c = 0; c < n; c++ )
                    {
                        m[r][c] -= m[i][c];
                        res[r][c] -= res[i][c];
                    }
            }
        }
    }

    for( i = 1; i < n; i++ )
        for( r = 0; r < i; r++ )
        {
            temp = m[r][i];
            for( c = 0; c < n; c++ )
            {
                m[r][c] -= (temp * m[i][c]);
                res[r][c] -= (temp * res[i][c]);
            }
        }

    for( r = 0; r < n; r++ )
        for( c = 0; c < n; c++ )
            m[r][c] = res[r][c];

    return rank;
}

