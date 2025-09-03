

#include <iostream>
#include <vector>
#include <string>
#include <cmath>
#include <stdexcept>
#include "rapidjson/document.h"
#include "rapidjson/filereadstream.h"

using namespace std;
using namespace rapidjson;

// FIX 1: Custom decoder to handle numbers larger than a long long.
// It converts the number string directly to a double to preserve magnitude.
double decodeBaseValue(const string& encodedValue, int base) {
    double result = 0.0;
    double powerOfBase = 1.0;

    for (int i = encodedValue.length() - 1; i >= 0; --i) {
        int digitValue = 0;
        char digitChar = encodedValue[i];

        if (digitChar >= '0' && digitChar <= '9') {
            digitValue = digitChar - '0';
        } else if (digitChar >= 'a' && digitChar <= 'f') {
            digitValue = 10 + (digitChar - 'a');
        } else if (digitChar >= 'A' && digitChar <= 'F') {
            digitValue = 10 + (digitChar - 'A');
        }
        
        result += digitValue * powerOfBase;
        powerOfBase *= base;
    }
    return result;
}

// FIX 2: Improved Gaussian elimination with pivoting for better numerical stability.
vector<double> performGaussianElimination(vector<vector<double>> coefficientMatrix, vector<double> constantsVector) {
    int n = coefficientMatrix.size();
    for (int pivot = 0; pivot < n; ++pivot) {
        // Find the row with the largest pivot element
        int max_row = pivot;
        for (int i = pivot + 1; i < n; i++) {
            if (abs(coefficientMatrix[i][pivot]) > abs(coefficientMatrix[max_row][pivot])) {
                max_row = i;
            }
        }
        // Swap rows to make the largest element the pivot
        swap(coefficientMatrix[pivot], coefficientMatrix[max_row]);
        swap(constantsVector[pivot], constantsVector[max_row]);

        // Normalize the pivot row
        double pivotElement = coefficientMatrix[pivot][pivot];
        for (int col = pivot; col < n; col++) {
            coefficientMatrix[pivot][col] /= pivotElement;
        }
        constantsVector[pivot] /= pivotElement;

        // Eliminate the current column in other rows
        for (int row = 0; row < n; row++) {
            if (row != pivot) {
                double factor = coefficientMatrix[row][pivot];
                for (int col = pivot; col < n; col++) {
                    coefficientMatrix[row][col] -= factor * coefficientMatrix[pivot][col];
                }
                constantsVector[row] -= factor * constantsVector[pivot];
            }
        }
    }
    return constantsVector;
}

void processTestCase(const char* filename) {
    FILE* filePointer = fopen(filename, "rb");
    if (!filePointer) {
        cerr << "Error: Unable to open " << filename << endl;
        return;
    }

    char buffer[65536];
    FileReadStream inputStream(filePointer, buffer, sizeof(buffer));
    Document jsonDocument;
    jsonDocument.ParseStream(inputStream);
    fclose(filePointer);

    if (jsonDocument.HasParseError()) {
        cerr << "Error: Failed to parse JSON in " << filename << endl;
        return;
    }

    const Value& keyData = jsonDocument["keys"];
    int polynomialDegree = keyData["k"].GetInt();

    // The matrix is correctly sized to k x k
    vector<vector<double>> coefficientMatrix(polynomialDegree, vector<double>(polynomialDegree, 0));
    vector<double> constantsVector(polynomialDegree, 0);

    // The loop correctly iterates k times to build a square system
    for (int i = 1; i <= polynomialDegree; ++i) {
        string index = to_string(i);
        const Value& rootData = jsonDocument[index.c_str()];
        
        // FIX 3: Safely read the 'base', whether it's a string or an integer.
        int base = 0;
        if (rootData["base"].IsInt()) {
            base = rootData["base"].GetInt();
        } else if (rootData["base"].IsString()) {
            base = stoi(rootData["base"].GetString());
        }

        string encodedValue = rootData["value"].GetString();
        
        // Use the new custom decoder and store the result in a double
        double decodedY = decodeBaseValue(encodedValue, base);
        
        constantsVector[i - 1] = decodedY;
        for (int j = 0; j < polynomialDegree; ++j) {
            coefficientMatrix[i - 1][j] = pow((double)i, polynomialDegree - j - 1);
        }
    }

    vector<double> solutionVector = performGaussianElimination(coefficientMatrix, constantsVector);

    cout << "The secret (constant term c) for " << filename << " is: " << round(solutionVector.back()) << endl;
}

int main() {
    processTestCase("input1.json");
    processTestCase("input2.json");
    return 0;

}
