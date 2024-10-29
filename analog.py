import math

# return in ms
def compute_t(C, R, thresh):
    # e^(-t/RC) = X
    X = 1 - thresh
    # t = -R.C.ln(X)
    t = -R * C * math.log(X)
    return t * 1e3

C = 22e-9
R = 1e6
thresh = 0.1
while thresh < 0.95:
    print('C=%.1e R=%.1e thresh=%0.2f max_t=%0.4f'
          % (C, R, thresh, compute_t(C, R, thresh)))
    thresh += 0.1
print()

R = 1e5
thresh = 0.1
while thresh < 0.95:
    print('C=%.1e R=%.1e thresh=%0.2f max_t=%0.4f'
          % (C, R, thresh, compute_t(C, R, thresh)))
    thresh += 0.1
print()

# t is proportional to ln(X). We use t as a proxy for R, and hence deflection.
# d/dx(ln(x)) = 1/x
# Measurement of x is accurate to 1 in 1000 (say). And range of x is 0-1.
# Error in t is: +/- (1e-3 * (1/x)) / ln(x) = +/- 1e-3 / (x * ln(x))
x = 0.1
while x < 0.95:
    error = -1e-3 / ((1-x) * math.log(1-x))
    print('thresh=%0.2f err=%0.2f%%' % (x, 100*error))
    x += 0.1
