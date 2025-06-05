// svm_rbf.h
#ifndef _SVM_RBF_H_
#define _SVM_RBF_H_

#include <ap_fixed.h>

// Define appropriate data types for precision and performance
typedef float data_t;

// Parameters
#define MAX_FEATURES 32  // Maximum number of features in each vector
#define MAX_SUPPORT_VECTORS 128  // Maximum number of support vectors

// Main function to compute RBF kernel for SVM
void svm_rbf_kernel(
    const data_t *x_test,           // Test vector
    const data_t *support_vectors,  // Support vectors
    const data_t *alphas,           // Alpha coefficients
    const data_t gamma,             // RBF kernel parameter
    const data_t bias,              // SVM bias term
    const int n_features,           // Number of features in each vector
    const int n_sv,                 // Number of support vectors
    data_t *decision_value);        // Output decision value

#endif