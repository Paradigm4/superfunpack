
#define M_LN_2PI       1.837877066409345483560659472811      /* log(2*pi) */
#define M_LN_SQRT_2PI  0.918938533204672741780329736406 /* log(sqrt(2*pi))*/
#define M_LN_SQRT_PId2 0.225791352644727432363097614947 /* log(sqrt(pi/2))*/

#ifdef __cplusplus
extern "C" {
#endif

double
stirlerr(double n);

double
bd0 (double x, double np);

double
dbinom_raw (double x, double n, double p, double q, int give_log);


double
fmax2(double x, double y);

double
dhyper (double x, double r, double b, double n, int give_log);

#ifdef __cplusplus
}
#endif
