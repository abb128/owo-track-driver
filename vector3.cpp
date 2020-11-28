
#include "vector3.h"
#include "basis.h"

void Vector3::rotate(const Vector3& p_axis, double p_phi) {
	*this = Basis(p_axis, p_phi).xform(*this);
}

Vector3 Vector3::rotated(const Vector3& p_axis, double p_phi) const {
	Vector3 r = *this;
	r.rotate(p_axis, p_phi);
	return r;
}

void Vector3::set_axis(int p_axis, double p_value) {
	coord[p_axis] = p_value;
}

double Vector3::get_axis(int p_axis) const {
	return operator[](p_axis);
}

int Vector3::min_axis() const {
	return x < y ? (x < z ? 0 : 2) : (y < z ? 1 : 2);
}

int Vector3::max_axis() const {
	return x < y ? (y < z ? 2 : 1) : (x < z ? 2 : 0);
}

/*
void Vector3::snap(Vector3 p_val) {
	x = Math::stepify(x, p_val.x);
	y = Math::stepify(y, p_val.y);
	z = Math::stepify(z, p_val.z);
}*/
/*
Vector3 Vector3::snapped(Vector3 p_val) const {
	Vector3 v = *this;
	v.snap(p_val);
	return v;
}
*/

Vector3 Vector3::cubic_interpolaten(const Vector3& p_b, const Vector3& p_pre_a, const Vector3& p_post_b, double p_t) const {
	Vector3 p0 = p_pre_a;
	Vector3 p1 = *this;
	Vector3 p2 = p_b;
	Vector3 p3 = p_post_b;

	{
		//normalize

		double ab = p0.distance_to(p1);
		double bc = p1.distance_to(p2);
		double cd = p2.distance_to(p3);

		if (ab > 0) {
			p0 = p1 + (p0 - p1) * (bc / ab);
		}
		if (cd > 0) {
			p3 = p2 + (p3 - p2) * (bc / cd);
		}
	}

	double t = p_t;
	double t2 = t * t;
	double t3 = t2 * t;

	Vector3 out;
	out = 0.5 * ((p1 * 2.0) +
		(-p0 + p2) * t +
		(2.0 * p0 - 5.0 * p1 + 4.0 * p2 - p3) * t2 +
		(-p0 + 3.0 * p1 - 3.0 * p2 + p3) * t3);
	return out;
}

Vector3 Vector3::cubic_interpolate(const Vector3& p_b, const Vector3& p_pre_a, const Vector3& p_post_b, double p_t) const {
	Vector3 p0 = p_pre_a;
	Vector3 p1 = *this;
	Vector3 p2 = p_b;
	Vector3 p3 = p_post_b;

	double t = p_t;
	double t2 = t * t;
	double t3 = t2 * t;

	Vector3 out;
	out = 0.5 * ((p1 * 2.0) +
		(-p0 + p2) * t +
		(2.0 * p0 - 5.0 * p1 + 4.0 * p2 - p3) * t2 +
		(-p0 + 3.0 * p1 - 3.0 * p2 + p3) * t3);
	return out;
}

Vector3 Vector3::move_toward(const Vector3& p_to, const double p_delta) const {
	Vector3 v = *this;
	Vector3 vd = p_to - v;
	double len = vd.length();
	return len <= p_delta || len < UNIT_EPSILON ? p_to : v + vd / len * p_delta;
}

Basis Vector3::outer(const Vector3& p_b) const {
	Vector3 row0(x * p_b.x, x * p_b.y, x * p_b.z);
	Vector3 row1(y * p_b.x, y * p_b.y, y * p_b.z);
	Vector3 row2(z * p_b.x, z * p_b.y, z * p_b.z);

	return Basis(row0, row1, row2);
}

Basis Vector3::to_diagonal_matrix() const {
	return Basis(x, 0, 0,
		0, y, 0,
		0, 0, z);
}

bool Vector3::is_equal_approx(const Vector3& p_v) const {
	return Math::is_equal_approx(x, p_v.x) && Math::is_equal_approx(y, p_v.y) && Math::is_equal_approx(z, p_v.z);
}