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

#ifndef FPLLL_HOUSEHOLDER_H
#define FPLLL_HOUSEHOLDER_H

#include "nr/matrix.h"

FPLLL_BEGIN_NAMESPACE

enum MatHouseholderFlags
{
  HOUSEHOLDER_DEFAULT  = 0,
  HOUSEHOLDER_ROW_EXPO = 1
};

/**
 * MatHouseholder provides an interface for performing elementary operations on a basis
 * and computing its R matrix using Householder transformations.
 */
template <class ZT, class FT> class MatHouseholder
{
public:
  /**
   * Constructor.
   * The precision of FT must be defined before creating an instance of the
   * class and must remain the same until the object is destroyed (or no longer
   * needed).
   * @param b
   *   The matrix on which row operations are performed. It must not be empty.
   */
  MatHouseholder(Matrix<ZT> &arg_b, int flags)
      : b(arg_b), enable_row_expo(flags & HOUSEHOLDER_ROW_EXPO)
  {
    d            = b.get_rows();
    n            = b.get_cols();
    n_known_rows = 0;
    sigma.resize(d);
    R.resize(d, n);
    V.resize(d, n);
    row_expo.resize(d);

    if (!enable_row_expo)
      fill(row_expo.begin(), row_expo.end(), -1);

#ifdef DEBUG
    for (int i = 0; i < d; i++)
    {
      for (int j = 0; j < n; j++)
      {
        V(i, j).set_nan();
      }
    }
#endif  // DEBUG
  }

  ~MatHouseholder() {}

  /**
   * Returns f = R(i, j).
   *
   * Returns reference to `f`.
   */
  inline void get_R(FT &f, int i, int j, long &expo);

  inline void set_R(FT &f, int i, int j);

  /**
   * Returns R[i].
   */
  inline MatrixRow<FT> get_R(int i, long &expo);

  /**
   * Returns the R matrix
   */
  const Matrix<FT> &get_R(vector<long> &expo)
  {
    expo = row_expo;
    return R;
  }

  /**
   * Returns b[i].
   */
  MatrixRow<ZT> get_b(int i);

  /**
   * Returns the b matrix
   */
  const Matrix<ZT> &get_b() { return b; }

  /**
   * Apply Householder transformation on row i, from cols 0 to last_j.
   * Restriction: last_j == i - 1 or i.
   */
  void update_R(int i, int last_j);

  /**
   * Apply Householder transformation on row i.
   */
  void update_R(int i);

  /**
   * Full computation of the matrix R.
   */
  inline void update_R();

  inline int get_d() { return d; }
  inline int get_n() { return n; }

  /**
   * Norm square of b[k].
   */
  inline void norm_square_b_row(FT &f, int k, long &expo);

  /**
   * Truncated norm square of R[k], with coefficients of R[k][0..end-1].
   */
  inline void norm_square_R_row(FT &f, int k, int end, long &expo);

  /**
   * b[k] = b[k] - sum_{i = 0}^{k - 1}(x[i] * b[i])
   */
  void add_mul_b_rows(int k, vector<FT> xf);

  /**
   * Swap row i and j of b.
   */
  inline void swap(int i, int j);

  /**
   * Invalidate row k to row n_known_rows - 1.
   * Update n_known_rows to k.
   */
  inline void invalidate_row(int k);

  inline bool is_enable_row_expo() { return enable_row_expo; }
private:
  /**
   * Number of rows of b (dimension of the lattice).
   */
  int d;

  /**
   * Number of columns of b (dimension of the lattice).
   */
  int n;

  /**
   * Basis of the lattice
   */
  Matrix<ZT> &b;

  /**
   * b = R * q_householder.
   * R is lower triangular and the diagonal coefficient are >= 0.
   */
  Matrix<FT> R;

  /**
   * Vector v following [MSV, ISSAC'09].
   */
  Matrix<FT> V;

  /**
   * Sigma values following [MSV, ISSAC'09].
   */
  vector<FT> sigma;

  /**
   * R[i] is invalid for i >= n_known_rows.
   */
  int n_known_rows;

  /** Normalization of each row of b by a power of 2. */
  const bool enable_row_expo;

  /**
   * When enable_row_expo=true, row_expo[i] is the smallest non-negative integer
   * such that b(i, j) &lt;= 2^row_expo[i] for all j. Otherwise this array is empty.
   */
  vector<long> row_expo;
};

template <class ZT, class FT>
inline void MatHouseholder<ZT, FT>::get_R(FT &f, int i, int j, long &expo)
{
  FPLLL_DEBUG_CHECK(i >= 0 && i < d && j >= 0 && j <= i);
  f    = R(i, j);
  expo = row_expo[i];
}

template <class ZT, class FT> inline MatrixRow<FT> MatHouseholder<ZT, FT>::get_R(int i, long &expo)
{
  FPLLL_DEBUG_CHECK(i >= 0 && i < d);
  expo = row_expo[i];

  return R[i];
}

template <class ZT, class FT> MatrixRow<ZT> MatHouseholder<ZT, FT>::get_b(int i)
{
  FPLLL_DEBUG_CHECK(i >= 0 && i < d);
  return b[i];
}

template <class ZT, class FT> inline void MatHouseholder<ZT, FT>::set_R(FT &f, int i, int j)
{
  FPLLL_DEBUG_CHECK(i >= 0 && i < d && i >= j && j >= 0);
  FPLLL_DEBUG_CHECK(j <= i);
  R(i, j) = f;
}

template <class ZT, class FT> inline void MatHouseholder<ZT, FT>::update_R(int i)
{
  update_R(i, i);
}

template <class ZT, class FT> inline void MatHouseholder<ZT, FT>::update_R()
{
  for (int i = 0; i < d; i++)
    update_R(i);
}

template <class ZT, class FT>
inline void MatHouseholder<ZT, FT>::norm_square_b_row(FT &f, int k, long &expo)
{
  FPLLL_DEBUG_CHECK(k >= 0 && k < d);
  ZT ztmp0;
  b[k].dot_product(ztmp0, b[k], 0, n);
  if (enable_row_expo)
    ztmp0.get_f_exp(f, expo);
  else
  {
    expo = -1;
    f.set_z(ztmp0);
  }
}

template <class ZT, class FT>
inline void MatHouseholder<ZT, FT>::norm_square_R_row(FT &f, int k, int end, long &expo)
{
  FPLLL_DEBUG_CHECK(k >= 0 && k < d);
  FPLLL_DEBUG_CHECK(0 <= end && end <= k);
  if (end == 0)
  {
    f = 0.0;
    FPLLL_DEBUG_CHECK(f.is_zero());
  }
  else
  {
    R[k].dot_product(f, R[k], 0, end);
  }
  expo = 2 * row_expo[k];
}

template <class ZT, class FT> inline void MatHouseholder<ZT, FT>::invalidate_row(int k)
{
#ifdef DEBUG
  if (k < n_known_rows)
    n_known_rows = k;
#endif  // DEBUG
}

template <class ZT, class FT> void MatHouseholder<ZT, FT>::swap(int i, int j)
{
  FPLLL_DEBUG_CHECK(0 <= i && i < j && j < d);

  invalidate_row(i);

  b.swap_rows(i, j);
}

FPLLL_END_NAMESPACE

#endif  // FPLLL_HOUSEHOLDER_H
