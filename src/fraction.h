#ifndef TIME_H
#define TIME_H

#include <numeric>
#include <iostream>

class Fraction {
    long long num;
    long long den;
public:
    Fraction(long long num = 0, long long den = 1) {
        if (den < 0) {
            num = -num;
            den = -den;
        }
        long long div = std::gcd(num, den);
        num /= div;
        den /= div;
        this->num = num;
        this->den = den;
    }

    bool isInt() const {
        return den == 1;
    }

    long long floor() const {
        return num / den; 
    }

    long long ceil() const {
        return (num + den - 1) / den;
    }

    long long getNum() const {
        return num;
    }

    long long getDen() const {
        return den;
    }

    float operator*() const {
        return (float)num / (float)den;
    }

    Fraction operator-() const {
        return Fraction(-num, den);
    }

    Fraction operator!() const {
        return Fraction(den, num);
    }

    Fraction operator+(Fraction other) const {
        return Fraction(num * other.den + den * other.num, den * other.den);
    }

    Fraction operator-(Fraction other) const {
        return (*this) + (-other);
    }

    Fraction operator*(Fraction other) const {
        return Fraction(num * other.num, den * other.den);
    }

    Fraction operator/(Fraction other) const {
        return (*this) * (!other);
    }

    bool operator==(Fraction other) const {
        return num == other.num && den == other.den;
    }

    bool operator!=(Fraction other) const {
        return !((*this) == other);
    }

    bool operator<(Fraction other) const {
        return num * other.den < other.num * den;
    }

    bool operator<=(Fraction other) const {
        return num * other.den <= other.num * den;
    }

    bool operator>(Fraction other) const {
        return num * other.den > other.num * den;
    }

    bool operator>=(Fraction other) const {
        return num * other.den >= other.num * den;
    }

    template<typename T>
    Fraction& operator+=(T value) {
        (*this) = (*this) + Fraction(value);
        return *this;
    }

    template<typename T>
    Fraction& operator-=(T value) {
        (*this) = (*this) - Fraction(value);
        return *this;
    }

    template<typename T>
    Fraction& operator*=(T value) {
        (*this) = (*this) * Fraction(value);
        return *this;
    }

    template<typename T>
    Fraction& operator/=(T value) {
        (*this) = (*this) / Fraction(value);
        return *this;
    }
};

template <typename T>
Fraction operator*(T value, Fraction frac) {
    return Fraction(value) * frac;
}

template <typename T>
Fraction operator*(Fraction frac, T value) {
    return frac * Fraction(value);
}

template <typename T>
Fraction operator/(T value, Fraction frac) {
    return Fraction(value) / frac;
}

template <typename T>
Fraction operator/(Fraction frac, T value) {
    return frac / Fraction(value);
}

template <typename T>
Fraction operator+(Fraction frac, T value) {
    return frac + Fraction(value);
}

template <typename T>
Fraction operator+(T value, Fraction frac) {
    return Fraction(value) + frac;
}

template <typename T>
Fraction operator-(T value, Fraction frac) {
    return Fraction(value) - frac;
}

template <typename T>
Fraction operator-(Fraction frac, T value) {
    return frac - Fraction(value);
}

template <typename T>
bool operator==(T value, Fraction frac) {
    return Fraction(value) == frac;
}

template <typename T>
bool operator==(Fraction frac, T value) {
    return frac == Fraction(value);
}

template <typename T>
bool operator!=(T value, Fraction frac) {
    return Fraction(value) != frac;
}

template <typename T>
bool operator!=(Fraction frac, T value) {
    return frac != Fraction(value);
}


template <typename T>
bool operator<(T value, Fraction frac) {
    return Fraction(value) < frac;
}

template <typename T>
bool operator<(Fraction frac, T value) {
    return frac < Fraction(value);
}

template <typename T>
bool operator<=(T value, Fraction frac) {
    return Fraction(value) <= frac;
}

template <typename T>
bool operator<=(Fraction frac, T value) {
    return frac <= Fraction(value);
}

template <typename T>
bool operator>(T value, Fraction frac) {
    return Fraction(value) > frac;
}

template <typename T>
bool operator>(Fraction frac, T value) {
    return frac > Fraction(value);
}

template <typename T>
bool operator>=(T value, Fraction frac) {
    return Fraction(value) >= frac;
}

template <typename T>
bool operator>=(Fraction frac, T value) {
    return frac >= Fraction(value);
}

#endif