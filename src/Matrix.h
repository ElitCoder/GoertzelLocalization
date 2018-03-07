#ifndef MATRIX_H
#define MATRIX_H

#include <iostream>
#include <cstddef>

#define ERROR(...)	do { fprintf(stderr, "Error: "); fprintf(stderr, __VA_ARGS__); fprintf(stderr, "\n"); exit(1); } while(0)

template<class T>
class Matrix {
public:
	Matrix(size_t n, size_t m) {
		n_ = n;
		m_ = m;
		
		for (size_t i = 0; i < n; i++)
			matrix_.push_back(std::vector<T>(m));
	}
	
	Matrix<T> transpose() {
		Matrix<T> transposed(n_, m_);
		
		#pragma omp parallel for
		for (size_t n = 0; n < n_ * m_; n++) {
			size_t i = n / n_;
			size_t j = n % n_;
			
			transposed[i][j] = matrix_[j][i];
		}
		
		return transposed;
	}
	
	size_t size() const {
		return matrix_.size();
	}
	
	std::vector<T>& operator[](size_t i) {
		return matrix_[i];
	}
	
	const std::vector<T>& operator[](size_t i) const {
		return matrix_[i];
	}
	
	Matrix operator-(const Matrix& matrix) {
		Matrix<T> negative(n_, m_);
		
		#pragma omp parallel for
		for (size_t i = 0; i < n_; i++)
			for (size_t j = 0; j < m_; j++)
				negative[i][j] = (*this)[i][j] - matrix[i][j];
		
		return negative;
	}
	
	Matrix operator*(double value) {
		Matrix<T> product(n_, m_);
		
		#pragma omp parallel for
		for (size_t i = 0; i < n_; i++)
			for (size_t j = 0; j < m_; j++)
				product[i][j] = matrix_[i][j] * value;
				
		return product;
	}
	
	friend std::ostream& operator<<(std::ostream& out, const Matrix& matrix) {
		//[ 1, 2, 3 ]
		//[ 2, 4, 6 ]
		
		using std::cout;
		using std::endl;
		
		for (size_t i = 0; i < matrix.size(); i++) {
			cout << "[ ";
			
			for (size_t j = 0; j < matrix[i].size(); j++) {
				cout << matrix[i][j];
				
				if (j + 1 != matrix[i].size())
					cout << ",";
				
				cout << " ";	
			}
			
			cout << "]";
			
			if (i + 1 != matrix.size())
				cout << endl;
		}
		
		return out;
	}
	
private:
	size_t n_;
	size_t m_;
	
	std::vector<std::vector<T>> matrix_;
};

#endif