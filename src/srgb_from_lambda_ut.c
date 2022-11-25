#include "srgb_from_lambda.h"

int main(int argc, char ** argv)
{
    if(argc != 2)
    {
        printf("Usage: %s lambda_nm\n", argv[0]);
        return EXIT_FAILURE;
    }

    double lambda = atof(argv[1]);

    double RGB[3];
    srgb_from_lambda(lambda, RGB);

    printf("lambda=%f nm approx. RGB %.2f, %.2f, %.2f or #%02X%02X%02X ",
           lambda,
            RGB[0], RGB[1], RGB[2],
            (int) round(255.0*RGB[0]),
            (int) round(255.0*RGB[1]),
            (int) round(255.0*RGB[2]));

    printf("\e[38;2;%d;%d;%dm\e[48;2;%d;%d;%dm   \e[0m\n",
            (int) round(255.0 - 255.0*RGB[0]),
            (int) round(255.0 - 255.0*RGB[1]),
            (int) round(255.0 - 255.0*RGB[2]),
            (int) round(255.0*RGB[0]),
            (int) round(255.0*RGB[1]),
            (int) round(255.0*RGB[2]));


    return EXIT_SUCCESS;
}
