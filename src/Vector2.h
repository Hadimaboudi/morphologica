/*!
 * A (templated) 2D vector class
 */
#ifndef _VECTOR2_H_
#define _VECTOR2_H_

#include <cmath>
using std::abs;
using std::sqrt;
using std::cos;
using std::sin;

#include <array>
using std::array;

#include <iostream>
using std::cout;
using std::endl;

namespace morph {

    template <class Flt>
    class Vector2
    {
    public:
        Vector2 (void)
            : x(0.0)
            , y(0.0) {}

        Vector2 (Flt _x, Flt _y)
            : x(_x)
            , y(_y) {}

        /*!
         * Vector components
         */
        //@{
        alignas(Flt) Flt x;
        alignas(Flt) Flt y;
        //@}

        void output (void) const {
            cout << "Vector2(" << x << "," << y << ")" << endl;
        }

        /*!
         * Renormalize the vector to 1
         */
        void renormalize (void) {
            Flt denom = sqrt (x*x + y*y);
            if (denom != static_cast<Flt>(0.0)) {
                Flt oneovermag = 1.0 / sqrt (x*x + y*y);
                this->x *= oneovermag;
                this->y *= oneovermag;
            }
        }

        /*!
         * The threshold outside of which the vector is no longer
         * considered to be a unit vector.
         */
        const Flt unitThresh = 0.001;

        /*!
         * Test to see if this vector is a unit vector (it doesn't *have* to be)
         */
        bool checkunit (void) {
            bool rtn = true;
            Flt metric = 1.0 - (x*x + y*y);
            if (abs(metric) > morph::Vector2<Flt>::unitThresh) {
                rtn = false;
            }
            return rtn;
        }

        //! Return the length of the vector
        Flt length (void) const {
            Flt len = sqrt (x*x + y*y);
            return len;
        }

        //! Assignment operator
        Vector2<Flt> operator= (const Vector2<Flt>& other) {
            Vector2<Flt> v;
            v.x = other.x;
            v.y = other.y;
            return v;
        }

        //! Scalar multiply.
        //@{
        Vector2<Flt> operator* (const float& f) {
            Vector2<Flt> v;
            v.x = this->x * static_cast<Flt>(f);
            v.y = this->y * static_cast<Flt>(f);
            return v;
        }
        Vector2<Flt> operator* (const double& d) {
            Vector2<Flt> v;
            v.x = this->x * static_cast<Flt>(d);
            v.y = this->y * static_cast<Flt>(d);
            return v;
        }

        //! Vector addition
        //@{
        Vector2<Flt> operator+ (const Vector2<Flt>& v2) {
            Vector2<Flt> v;
            v.x = this->x + v2.x;
            v.y = this->y + v2.y;
            return v;
        }
        Vector2<Flt> operator+= (const Vector2<Flt>& v2) {
            this->x = this->x + v2.x;
            this->y = this->y + v2.y;
            return *this;
        }
        //@}

        //! Vector subtraction
        //@{
        Vector2<Flt> operator- (const Vector2<Flt>& v2) {
            Vector2<Flt> v;
            v.x = this->x - v2.x;
            v.y = this->y - v2.y;
            return v;
        }
        Vector2<Flt> operator-= (const Vector2<Flt>& v2) {
            this->x = this->x - v2.x;
            this->y = this->y - v2.y;
            return *this;
        }
        //@}

        //! Scalar addition
        //@{
        Vector2<Flt> operator+ (const float& f) {
            Vector2<Flt> v;
            v.x = this->x + static_cast<Flt>(f);
            v.y = this->y + static_cast<Flt>(f);
            return v;
        }
        Vector2<Flt> operator+ (const double& d) {
            Vector2<Flt> v;
            v.x = this->x + static_cast<Flt>(d);
            v.y = this->y + static_cast<Flt>(d);
            return v;
        }
        //@}

        //! Scalar subtraction
        //@{
        Vector2<Flt> operator- (const float& f) {
            Vector2<Flt> v;
            v.x = this->x - static_cast<Flt>(f);
            v.y = this->y - static_cast<Flt>(f);
            return v;
        }
        Vector2<Flt> operator- (const double& d) {
            Vector2<Flt> v;
            v.x = this->x - static_cast<Flt>(d);
            v.y = this->y - static_cast<Flt>(d);
            return v;
        }
        //@}
    };

} // namespace morph

#endif // _VECTOR2_H_
