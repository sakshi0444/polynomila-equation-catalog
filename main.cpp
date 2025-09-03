

#include <iostream>
#include <vector>
#include <string>
#include <cmath>
#include <stdexcept>
#include <algorithm>
#include "rapidjson/document.h"
#include "rapidjson/filereadstream.h"

using namespace std;
using namespace rapidjson;

// Use long long for better precision with large integers
typedef long long ll;

// Structure to represent a rational number for exact arithmetic
struct Fraction {
    ll num, den;
    
    Fraction(ll n = 0, ll d = 1) : num(n), den(d) {
        if (den < 0) {
            num = -num;
            den = -den;
        }
        simplify();
    }
    
    ll gcd(ll a, ll b) {
        if (b == 0) return a;
        return gcd(b, a % b);
    }
    
    void simplify() {
        if (den == 0) return;
        ll g = gcd(abs(num), abs(den));
        num /= g;
        den /= g;
    }
    
    Fraction operator+(const Fraction& other) const {
        return Fraction(num * other.den + other.num * den, den * other.den);
    }
    
    Fraction operator-(const Fraction& other) const {
        return Fraction(num * other.den - other.num * den, den * other.den);
    }
    
    Fraction operator*(const Fraction& other) const {
        return Fraction(num * other.num, den * other.den);
    }
    
    Fraction operator/(const Fraction& other) const {
        return Fraction(num * other.den, den * other.num);
    }
    
    double toDouble() const {
        return (double)num / den;
    }
};

// Convert from any base to decimal using string arithmetic for large numbers
ll decodeBaseValue(const string& encodedValue, int base) {
    ll result = 0;
    ll powerOfBase = 1;

    for (int i = encodedValue.length() - 1; i >= 0; --i) {
        int digitValue = 0;
        char digitChar = encodedValue[i];

        if (digitChar >= '0' && digitChar <= '9') {
            digitValue = digitChar - '0';
        } else if (digitChar >= 'a' && digitChar <= 'z') {
            digitValue = 10 + (digitChar - 'a');
        } else if (digitChar >= 'A' && digitChar <= 'Z') {
            digitValue = 10 + (digitChar - 'A');
        }
        
        if (digitValue >= base) {
            throw invalid_argument("Invalid digit for given base");
        }
        
        result += digitValue * powerOfBase;
        powerOfBase *= base;
    }
    return result;
}

// Lagrange interpolation to find f(0) - the secret
ll lagrangeInterpolation(const vector<pair<int, ll>>& points) {
    Fraction result(0, 1);
    int k = points.size();
    
    for (int i = 0; i < k; i++) {
        // Calculate Lagrange basis polynomial L_i(0)
        Fraction basisValue(1, 1);
        
        for (int j = 0; j < k; j++) {
            if (i != j) {
                // L_i(0) = Product of (0 - x_j) / (x_i - x_j) for all j != i
                Fraction numerator(-points[j].first, 1);  // (0 - x_j)
                Fraction denominator(points[i].first - points[j].first, 1);  // (x_i - x_j)
                basisValue = basisValue * (numerator / denominator);
            }
        }
        
        // Add y_i * L_i(0) to result
        Fraction term(points[i].second, 1);
        result = result + (term * basisValue);
    }
    
    // Result should be an integer for Shamir's Secret Sharing
    if (result.den != 1) {
        cerr << "Warning: Result is not an integer: " << result.num << "/" << result.den << endl;
    }
    
    return result.num;
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
    int k = keyData["k"].GetInt();  // threshold - minimum shares needed
    int n = keyData["n"].GetInt();  // total shares available

    vector<pair<int, ll>> shares;  // (x, y) pairs

    // Collect all available shares
    for (auto& member : jsonDocument.GetObject()) {
        if (strcmp(member.name.GetString(), "keys") != 0) {
            int x = stoi(member.name.GetString());
            
            const Value& shareData = member.value;
            
            // Read base (handle both string and integer format)
            int base = 0;
            if (shareData["base"].IsInt()) {
                base = shareData["base"].GetInt();
            } else if (shareData["base"].IsString()) {
                base = stoi(shareData["base"].GetString());
            }

            string encodedValue = shareData["value"].GetString();
            
            try {
                ll decodedY = decodeBaseValue(encodedValue, base);
                shares.push_back({x, decodedY});
            } catch (const exception& e) {
                cerr << "Error decoding share " << x << ": " << e.what() << endl;
                continue;
            }
        }
    }

    if (shares.size() < k) {
        cerr << "Error: Not enough shares to reconstruct secret. Need " << k 
             << ", got " << shares.size() << endl;
        return;
    }

    // Sort shares by x value for consistency
    sort(shares.begin(), shares.end());

    // Use first k shares for reconstruction
    vector<pair<int, ll>> selectedShares(shares.begin(), shares.begin() + k);

    // Apply Lagrange interpolation to find f(0) - the secret
    ll secret = lagrangeInterpolation(selectedShares);

    cout << "The secret (constant term c) for " << filename << " is: " << secret << endl;
}

int main() {
    processTestCase("input1.json");
    processTestCase("input2.json");
    return 0;

}
