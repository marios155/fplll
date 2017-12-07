/* Copyright (C) 2005-2008 Damien Stehle.
   Copyright (C) 2007 David Cade.
   Copyright (C) 2011 Xavier Pujol.

   This file is part of fplll. fplll is free software: you
   can redistribute it and/or modify it under the terms of the GNU Lesser
   General Public License as published by the Free Software Foundation,
   either version 2.1 of the License, or (at your option) any later version.

   fplll is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
   GNU Lesser General Public License for more details.

   You should have received a copy of the GNU Lesser General Public License
   along with fplll. If not, see <http://www.gnu.org/licenses/>. */

/* Template source file */

#include "hlll.h"
#include "util.h"

FPLLL_BEGIN_NAMESPACE

template <class ZT, class FT> void HLLLReduction<ZT, FT>::lll()
{
  int k     = 1;
  int k_max = 0;
  FT s;
  FT tmp;
  FT sum;
  FT delta_           = delta;  // TODO: not exactly the good value
  bool update_R_row_0 = true;
  int start_time      = cputime();
  long expo_k1_k1, expo_k_k1, expo_k_k;

  if (verbose)
    print_params();

  while (k < m.get_d())
  {
    if (update_R_row_0)
    {
      m.update_R(0);
      update_R_row_0 = !update_R_row_0;
    }
    if (k > k_max)
    {
      if (verbose)
      {
        cerr << "Discovering vector " << k + 1 << "/" << m.get_d()
             << " cputime=" << cputime() - start_time << endl;
      }
      k_max = k;
    }

    size_reduction(k);

    m.get_R(s, k, k - 1, expo_k_k1);
    s.mul(s, s);  // s = R(k, k - 1)^2
    m.get_R(tmp, k, k, expo_k_k);
    tmp.mul(tmp, tmp);  // tmp = R(k, k)^2
    s.add(tmp, s);      // s = R(k, k - 1)^2 + R(k, k)^2
    m.get_R(tmp, k - 1, k - 1, expo_k1_k1);
    tmp.mul(tmp, tmp);
    tmp.mul(delta_, tmp);  // tmp = delta_ * R(k - 1, k - 1)^2

    if (expo_k1_k1 > -1)
      s.mul_2si(s, 2 * (expo_k_k - expo_k1_k1));
    if (tmp <= s)
      k++;
    else
    {
      if (k - 1 == 0)
        update_R_row_0 = !update_R_row_0;
      m.swap(k - 1, k);
      k = max(k - 1, 1);
    }
  }
}

template <class ZT, class FT> void HLLLReduction<ZT, FT>::size_reduction(int k)
{
  vector<FT> xf(k);
  FT ftmp0, ftmp1;
  long expo0, expo1;

  do
  {
    m.update_R(k, k - 1);
    for (int i = k - 1; i >= 0; i--)
    {
      m.get_R(xf[i], k, i, expo0);  // expo0 = row_expo[k]
      m.get_R(ftmp0, i, i, expo1);  // expo1 = row_expo[i]

      xf[i].div(xf[i], ftmp0);  // x[i] = R(k, i) / R(i, i)
      /* If T = mpfr or dpe, enable_row_expo must be false and then, expo0 - expo1 == 0 (required by
       * rnd_we with this
       * types) */
      xf[i].rnd_we(xf[i], expo0 - expo1);
      xf[i].neg(xf[i]);

      if (xf[i].cmp(0.0) != 0)
      {
        for (int j = 0; j < i; j++)
        {
          m.get_R(ftmp1, i, j, expo0);  // expo0 = row_expo[i]
          ftmp1.mul(xf[i], ftmp1);      // ftmp1 = x[i] * R(i, j)
          m.get_R(ftmp0, k, j, expo0);  // expo0 = row_expo[k]
          ftmp0.add(ftmp0, ftmp1);      // ftmp0 = R(k, j) + x[i] * R(i, j)
          m.set_R(ftmp0, k, j);
        }
      }
    }
    m.norm_square_b_row(ftmp1, k, expo0);  // ftmp1 = ||b[k]||^2
    m.add_mul_b_rows(k, xf);
    m.norm_square_b_row(ftmp0, k, expo1);  // ftmp0 = ||b[k]||^2
    ftmp1.mul(sr, ftmp1);                  // ftmp1 = 2^(-cd) * ftmp1
    if (expo1 > -1)
      ftmp0.mul_2si(ftmp0, expo1 - expo0);
  } while (ftmp0.cmp(ftmp1) <= 0);

  m.update_R(k);
}

// Works only there is no row_expo.
template <class ZT, class FT>
bool is_hlll_reduced(MatHouseholder<ZT, FT> &m, double delta, double eta)
{
  FT ftmp0;
  FT ftmp1;
  FT delta_ = delta;
  m.update_R();
  long expo;

  for (int i = 0; i < m.get_d(); i++)
  {
    for (int j = 0; j < i; j++)
    {
      m.get_R(ftmp0, i, j, expo);
      m.get_R(ftmp1, j, j, expo);
      ftmp1.div(ftmp0, ftmp1);
      ftmp1.abs(ftmp1);
      if (ftmp1.cmp(0.5) > 0)
        return false;
    }
  }
  for (int i = 1; i < m.get_d(); i++)
  {
    m.norm_square_b_row(ftmp0, i, expo);  // ftmp0 = ||b[i]||^2
    m.norm_square_R_row(ftmp1, i, i - 1, expo);
    ftmp1.sub(ftmp0, ftmp1);  // ftmp1 = ||b[i]||^2 - sum_{i = 0}^{i < i - 1}R[i][i]^2
    m.get_R(ftmp0, i - 1, i - 1, expo);
    ftmp0.mul(ftmp0, ftmp0);
    ftmp0.mul(delta_, ftmp0);  // ftmp0 = delta_ * R(i - 1, i - 1)^2

    if (ftmp0.cmp(ftmp1) > 0)
      return false;
  }
  return true;
}

/** instantiate functions **/

template class HLLLReduction<Z_NR<long>, FP_NR<double>>;
template class HLLLReduction<Z_NR<double>, FP_NR<double>>;
template class HLLLReduction<Z_NR<mpz_t>, FP_NR<double>>;
template bool
is_hlll_reduced<Z_NR<mpz_t>, FP_NR<double>>(MatHouseholder<Z_NR<mpz_t>, FP_NR<double>> &m,
                                            double delta, double eta);
template bool
is_hlll_reduced<Z_NR<long>, FP_NR<double>>(MatHouseholder<Z_NR<long>, FP_NR<double>> &m,
                                           double delta, double eta);
template bool
is_hlll_reduced<Z_NR<double>, FP_NR<double>>(MatHouseholder<Z_NR<double>, FP_NR<double>> &m,
                                             double delta, double eta);

#ifdef FPLLL_WITH_LONG_DOUBLE
template class HLLLReduction<Z_NR<long>, FP_NR<long double>>;
template class HLLLReduction<Z_NR<double>, FP_NR<long double>>;
template class HLLLReduction<Z_NR<mpz_t>, FP_NR<long double>>;
template bool
is_hlll_reduced<Z_NR<mpz_t>, FP_NR<long double>>(MatHouseholder<Z_NR<mpz_t>, FP_NR<long double>> &m,
                                                 double delta, double eta);
template bool
is_hlll_reduced<Z_NR<long>, FP_NR<long double>>(MatHouseholder<Z_NR<long>, FP_NR<long double>> &m,
                                                double delta, double eta);
template bool is_hlll_reduced<Z_NR<double>, FP_NR<long double>>(
    MatHouseholder<Z_NR<double>, FP_NR<long double>> &m, double delta, double eta);
#endif

#ifdef FPLLL_WITH_QD
template class HLLLReduction<Z_NR<long>, FP_NR<dd_real>>;
template class HLLLReduction<Z_NR<double>, FP_NR<dd_real>>;
template class HLLLReduction<Z_NR<mpz_t>, FP_NR<dd_real>>;

template class HLLLReduction<Z_NR<long>, FP_NR<qd_real>>;
template class HLLLReduction<Z_NR<double>, FP_NR<qd_real>>;
template class HLLLReduction<Z_NR<mpz_t>, FP_NR<qd_real>>;
template bool
is_hlll_reduced<Z_NR<mpz_t>, FP_NR<qd_real>>(MatHouseholder<Z_NR<mpz_t>, FP_NR<qd_real>> &m,
                                             double delta, double eta);
template bool
is_hlll_reduced<Z_NR<long>, FP_NR<qd_real>>(MatHouseholder<Z_NR<long>, FP_NR<qd_real>> &m,
                                            double delta, double eta);
template bool
is_hlll_reduced<Z_NR<double>, FP_NR<qd_real>>(MatHouseholder<Z_NR<double>, FP_NR<qd_real>> &m,
                                              double delta, double eta);
template bool
is_hlll_reduced<Z_NR<mpz_t>, FP_NR<dd_real>>(MatHouseholder<Z_NR<mpz_t>, FP_NR<dd_real>> &m,
                                             double delta, double eta);
template bool
is_hlll_reduced<Z_NR<long>, FP_NR<dd_real>>(MatHouseholder<Z_NR<long>, FP_NR<dd_real>> &m,
                                            double delta, double eta);
template bool
is_hlll_reduced<Z_NR<double>, FP_NR<dd_real>>(MatHouseholder<Z_NR<double>, FP_NR<dd_real>> &m,
                                              double delta, double eta);
#endif

#ifdef FPLLL_WITH_DPE
template class HLLLReduction<Z_NR<long>, FP_NR<dpe_t>>;
template class HLLLReduction<Z_NR<double>, FP_NR<dpe_t>>;
template class HLLLReduction<Z_NR<mpz_t>, FP_NR<dpe_t>>;
template bool
is_hlll_reduced<Z_NR<mpz_t>, FP_NR<dpe_t>>(MatHouseholder<Z_NR<mpz_t>, FP_NR<dpe_t>> &m,
                                           double delta, double eta);
template bool is_hlll_reduced<Z_NR<long>, FP_NR<dpe_t>>(MatHouseholder<Z_NR<long>, FP_NR<dpe_t>> &m,
                                                        double delta, double eta);
template bool
is_hlll_reduced<Z_NR<double>, FP_NR<dpe_t>>(MatHouseholder<Z_NR<double>, FP_NR<dpe_t>> &m,
                                            double delta, double eta);
#endif

template class HLLLReduction<Z_NR<long>, FP_NR<mpfr_t>>;
template class HLLLReduction<Z_NR<double>, FP_NR<mpfr_t>>;
template class HLLLReduction<Z_NR<mpz_t>, FP_NR<mpfr_t>>;
template bool
is_hlll_reduced<Z_NR<mpz_t>, FP_NR<mpfr_t>>(MatHouseholder<Z_NR<mpz_t>, FP_NR<mpfr_t>> &m,
                                            double delta, double eta);
template bool
is_hlll_reduced<Z_NR<long>, FP_NR<mpfr_t>>(MatHouseholder<Z_NR<long>, FP_NR<mpfr_t>> &m,
                                           double delta, double eta);
template bool
is_hlll_reduced<Z_NR<double>, FP_NR<mpfr_t>>(MatHouseholder<Z_NR<double>, FP_NR<mpfr_t>> &m,
                                             double delta, double eta);

FPLLL_END_NAMESPACE
