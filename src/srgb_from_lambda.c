#include "srgb_from_lambda.h"

static double g(double lambda, double mu, double s_1, double s_2)
{
    double s2 = s_1*s_2;
    if(lambda > mu)
    {
        s2 = s_2*s_2;
    }

    return exp(-1.0/2.0*(lambda - mu)*(lambda - mu)/s2);
}

static double sRGB_from_RGB(double V)
{

    if(V <= 0.0031308)
    {
        V = 324.0*V/25.0;
    }
    else
    {
        V = (211.0*pow(V,5.0/12.0) -11.0)/200.0;
    }

    V < 0 ? V = 0 : 0;
    V > 1 ? V = 1 : 0;

    return V;
}

void srgb_from_lambda(double lambda, double * RGB)
{
    /* Approximation of CIE 1931 data, found at:
     * https://en.wikipedia.org/wiki/CIE_1931_color_space
     */


    double X = 1.056 * g(lambda, 599.8, 37.9, 31.0)
        + 0.362*g(lambda, 442.0, 16.0, 26.7)
        - 0.065*g(lambda, 501.1, 20.4, 26.2);
    double Y = 0.821*g(lambda, 568.8, 46.9, 40.5)
        + 0.286*g(lambda, 530.9, 16.3, 31.1);
    double Z = 1.217*g(lambda, 437.0, 11.8, 36.0)
        + 0.681*g(lambda, 459.0, 26.0, 13.8);

    /* Linear conversion from XYZ to RGB
     * https://www.oceanopticsbook.info/view/photometry-and-visibility/from-xyz-to-rgb
     * http://www.brucelindbloom.com/ */
    double R =  3.2404542*X - 1.5371385*Y - 0.4985314*Z;
    double G = -0.9692660*X + 1.8760108*Y + 0.0415560*Z;
    double B =  0.0556434*X - 0.2040259*Y + 1.0572252*Z;

    R = sRGB_from_RGB(R);
    G = sRGB_from_RGB(G);
    B = sRGB_from_RGB(B);

    RGB[0] = R;
    RGB[1] = G;
    RGB[2] = B;
    return;
}
