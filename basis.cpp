
#include "basis.h"

#define cofac(row1, col1, row2, col2) \
	(elements[row1][col1] * elements[row2][col2] - elements[row1][col2] * elements[row2][col1])

void Basis::from_z(const Vector3& p_z) {
	if (std::abs(p_z.z) > Math_SQRT12) {
		// choose p in y-z plane
		double a = p_z[1] * p_z[1] + p_z[2] * p_z[2];
		double k = 1.0 / std::sqrt(a);
		elements[0] = Vector3(0, -p_z[2] * k, p_z[1] * k);
		elements[1] = Vector3(a * k, -p_z[0] * elements[0][2], p_z[0] * elements[0][1]);
	}
	else {
		// choose p in x-y plane
		double a = p_z.x * p_z.x + p_z.y * p_z.y;
		double k = 1.0 / std::sqrt(a);
		elements[0] = Vector3(-p_z.y * k, p_z.x * k, 0);
		elements[1] = Vector3(-p_z.z * elements[0].y, p_z.z * elements[0].x, a * k);
	}
	elements[2] = p_z;
}

void Basis::invert() {
	double co[3] = {
		cofac(1, 1, 2, 2), cofac(1, 2, 2, 0), cofac(1, 0, 2, 1)
	};
	double det = elements[0][0] * co[0] +
		elements[0][1] * co[1] +
		elements[0][2] * co[2];
#ifdef MATH_CHECKS
	ERR_FAIL_COND(det == 0);
#endif
	double s = 1.0 / det;

	set(co[0] * s, cofac(0, 2, 2, 1) * s, cofac(0, 1, 1, 2) * s,
		co[1] * s, cofac(0, 0, 2, 2) * s, cofac(0, 2, 1, 0) * s,
		co[2] * s, cofac(0, 1, 2, 0) * s, cofac(0, 0, 1, 1) * s);
}

void Basis::orthonormalize() {
	// Gram-Schmidt Process

	Vector3 x = get_axis(0);
	Vector3 y = get_axis(1);
	Vector3 z = get_axis(2);

	x.normalize();
	y = (y - x * (x.dot(y)));
	y.normalize();
	z = (z - x * (x.dot(z)) - y * (y.dot(z)));
	z.normalize();

	set_axis(0, x);
	set_axis(1, y);
	set_axis(2, z);
}

Basis Basis::orthonormalized() const {
	Basis c = *this;
	c.orthonormalize();
	return c;
}

bool Basis::is_orthogonal() const {
	Basis identity;
	Basis m = (*this) * transposed();

	return m.is_equal_approx(identity);
}

bool Basis::is_diagonal() const {
	return (
		Math::is_zero_approx(elements[0][1]) && Math::is_zero_approx(elements[0][2]) &&
		Math::is_zero_approx(elements[1][0]) && Math::is_zero_approx(elements[1][2]) &&
		Math::is_zero_approx(elements[2][0]) && Math::is_zero_approx(elements[2][1]));
}

bool Basis::is_rotation() const {
	return Math::is_equal_approx(determinant(), 1, UNIT_EPSILON) && is_orthogonal();
}

#ifdef MATH_CHECKS
// This method is only used once, in diagonalize. If it's desired elsewhere, feel free to remove the #ifdef.
bool Basis::is_symmetric() const {
	if (!Math::is_equal_approx(elements[0][1], elements[1][0])) {
		return false;
	}
	if (!Math::is_equal_approx(elements[0][2], elements[2][0])) {
		return false;
	}
	if (!Math::is_equal_approx(elements[1][2], elements[2][1])) {
		return false;
	}

	return true;
}
#endif

Basis Basis::diagonalize() {
	//NOTE: only implemented for symmetric matrices
	//with the Jacobi iterative method method
#ifdef MATH_CHECKS
	ERR_FAIL_COND_V(!is_symmetric(), Basis());
#endif
	const int ite_max = 1024;

	double off_matrix_norm_2 = elements[0][1] * elements[0][1] + elements[0][2] * elements[0][2] + elements[1][2] * elements[1][2];

	int ite = 0;
	Basis acc_rot;
	while (off_matrix_norm_2 > UNIT_EPSILON && ite++ < ite_max) {
		double el01_2 = elements[0][1] * elements[0][1];
		double el02_2 = elements[0][2] * elements[0][2];
		double el12_2 = elements[1][2] * elements[1][2];
		// Find the pivot element
		int i, j;
		if (el01_2 > el02_2) {
			if (el12_2 > el01_2) {
				i = 1;
				j = 2;
			}
			else {
				i = 0;
				j = 1;
			}
		}
		else {
			if (el12_2 > el02_2) {
				i = 1;
				j = 2;
			}
			else {
				i = 0;
				j = 2;
			}
		}

		// Compute the rotation angle
		double angle;
		if (Math::is_equal_approx(elements[j][j], elements[i][i])) {
			angle = Math_PI / 4;
		}
		else {
			angle = 0.5 * std::atan(2 * elements[i][j] / (elements[j][j] - elements[i][i]));
		}

		// Compute the rotation matrix
		Basis rot;
		rot.elements[i][i] = rot.elements[j][j] = std::cos(angle);
		rot.elements[i][j] = -(rot.elements[j][i] = std::sin(angle));

		// Update the off matrix norm
		off_matrix_norm_2 -= elements[i][j] * elements[i][j];

		// Apply the rotation
		*this = rot * *this * rot.transposed();
		acc_rot = rot * acc_rot;
	}

	return acc_rot;
}

Basis Basis::inverse() const {
	Basis inv = *this;
	inv.invert();
	return inv;
}

void Basis::transpose() {
	SWAP(elements[0][1], elements[1][0]);
	SWAP(elements[0][2], elements[2][0]);
	SWAP(elements[1][2], elements[2][1]);
}

Basis Basis::transposed() const {
	Basis tr = *this;
	tr.transpose();
	return tr;
}

// Multiplies the matrix from left by the scaling matrix: M -> S.M
// See the comment for Basis::rotated for further explanation.
void Basis::scale(const Vector3& p_scale) {
	elements[0][0] *= p_scale.x;
	elements[0][1] *= p_scale.x;
	elements[0][2] *= p_scale.x;
	elements[1][0] *= p_scale.y;
	elements[1][1] *= p_scale.y;
	elements[1][2] *= p_scale.y;
	elements[2][0] *= p_scale.z;
	elements[2][1] *= p_scale.z;
	elements[2][2] *= p_scale.z;
}

Basis Basis::scaled(const Vector3& p_scale) const {
	Basis m = *this;
	m.scale(p_scale);
	return m;
}

void Basis::scale_local(const Vector3& p_scale) {
	// performs a scaling in object-local coordinate system:
	// M -> (M.S.Minv).M = M.S.
	*this = scaled_local(p_scale);
}

float Basis::get_uniform_scale() const {
	return (elements[0].length() + elements[1].length() + elements[2].length()) / 3.0;
}

void Basis::make_scale_uniform() {
	float l = (elements[0].length() + elements[1].length() + elements[2].length()) / 3.0;
	for (int i = 0; i < 3; i++) {
		elements[i].normalize();
		elements[i] *= l;
	}
}

Basis Basis::scaled_local(const Vector3& p_scale) const {
	Basis b;
	b.set_diagonal(p_scale);

	return (*this) * b;
}

Vector3 Basis::get_scale_abs() const {
	return Vector3(
		Vector3(elements[0][0], elements[1][0], elements[2][0]).length(),
		Vector3(elements[0][1], elements[1][1], elements[2][1]).length(),
		Vector3(elements[0][2], elements[1][2], elements[2][2]).length());
}

Vector3 Basis::get_scale_local() const {
	double det_sign = Math::sign(determinant());
	return det_sign * Vector3(elements[0].length(), elements[1].length(), elements[2].length());
}

// get_scale works with get_rotation, use get_scale_abs if you need to enforce positive signature.
Vector3 Basis::get_scale() const {
	// FIXME: We are assuming M = R.S (R is rotation and S is scaling), and use polar decomposition to extract R and S.
	// A polar decomposition is M = O.P, where O is an orthogonal matrix (meaning rotation and reflection) and
	// P is a positive semi-definite matrix (meaning it contains absolute values of scaling along its diagonal).
	//
	// Despite being different from what we want to achieve, we can nevertheless make use of polar decomposition
	// here as follows. We can split O into a rotation and a reflection as O = R.Q, and obtain M = R.S where
	// we defined S = Q.P. Now, R is a proper rotation matrix and S is a (signed) scaling matrix,
	// which can involve negative scalings. However, there is a catch: unlike the polar decomposition of M = O.P,
	// the decomposition of O into a rotation and reflection matrix as O = R.Q is not unique.
	// Therefore, we are going to do this decomposition by sticking to a particular convention.
	// This may lead to confusion for some users though.
	//
	// The convention we use here is to absorb the sign flip into the scaling matrix.
	// The same convention is also used in other similar functions such as get_rotation_axis_angle, get_rotation, ...
	//
	// A proper way to get rid of this issue would be to store the scaling values (or at least their signs)
	// as a part of Basis. However, if we go that path, we need to disable direct (write) access to the
	// matrix elements.
	//
	// The rotation part of this decomposition is returned by get_rotation* functions.
	double det_sign = Math::sign(determinant());
	return det_sign * Vector3(
		Vector3(elements[0][0], elements[1][0], elements[2][0]).length(),
		Vector3(elements[0][1], elements[1][1], elements[2][1]).length(),
		Vector3(elements[0][2], elements[1][2], elements[2][2]).length());
}

// Decomposes a Basis into a rotation-reflection matrix (an element of the group O(3)) and a positive scaling matrix as B = O.S.
// Returns the rotation-reflection matrix via reference argument, and scaling information is returned as a Vector3.
// This (internal) function is too specific and named too ugly to expose to users, and probably there's no need to do so.
Vector3 Basis::rotref_posscale_decomposition(Basis& rotref) const {
#ifdef MATH_CHECKS
	ERR_FAIL_COND_V(determinant() == 0, Vector3());

	Basis m = transposed() * (*this);
	ERR_FAIL_COND_V(!m.is_diagonal(), Vector3());
#endif
	Vector3 scale = get_scale();
	Basis inv_scale = Basis().scaled(scale.inverse()); // this will also absorb the sign of scale
	rotref = (*this) * inv_scale;

#ifdef MATH_CHECKS
	ERR_FAIL_COND_V(!rotref.is_orthogonal(), Vector3());
#endif
	return scale.abs();
}

// Multiplies the matrix from left by the rotation matrix: M -> R.M
// Note that this does *not* rotate the matrix itself.
//
// The main use of Basis is as Transform.basis, which is used a the transformation matrix
// of 3D object. Rotate here refers to rotation of the object (which is R * (*this)),
// not the matrix itself (which is R * (*this) * R.transposed()).
Basis Basis::rotated(const Vector3& p_axis, double p_phi) const {
	return Basis(p_axis, p_phi) * (*this);
}

void Basis::rotate(const Vector3& p_axis, double p_phi) {
	*this = rotated(p_axis, p_phi);
}

void Basis::rotate_local(const Vector3& p_axis, double p_phi) {
	// performs a rotation in object-local coordinate system:
	// M -> (M.R.Minv).M = M.R.
	*this = rotated_local(p_axis, p_phi);
}

Basis Basis::rotated_local(const Vector3& p_axis, double p_phi) const {
	return (*this) * Basis(p_axis, p_phi);
}

Basis Basis::rotated(const Vector3& p_euler) const {
	return Basis(p_euler) * (*this);
}

void Basis::rotate(const Vector3& p_euler) {
	*this = rotated(p_euler);
}

Basis Basis::rotated(const Quat& p_quat) const {
	return Basis(p_quat) * (*this);
}

void Basis::rotate(const Quat& p_quat) {
	*this = rotated(p_quat);
}

Vector3 Basis::get_rotation_euler() const {
	// Assumes that the matrix can be decomposed into a proper rotation and scaling matrix as M = R.S,
	// and returns the Euler angles corresponding to the rotation part, complementing get_scale().
	// See the comment in get_scale() for further information.
	Basis m = orthonormalized();
	double det = m.determinant();
	if (det < 0) {
		// Ensure that the determinant is 1, such that result is a proper rotation matrix which can be represented by Euler angles.
		m.scale(Vector3(-1, -1, -1));
	}

	return m.get_euler();
}

Quat Basis::get_rotation_quat() const {
	// Assumes that the matrix can be decomposed into a proper rotation and scaling matrix as M = R.S,
	// and returns the Euler angles corresponding to the rotation part, complementing get_scale().
	// See the comment in get_scale() for further information.
	Basis m = orthonormalized();
	double det = m.determinant();
	if (det < 0) {
		// Ensure that the determinant is 1, such that result is a proper rotation matrix which can be represented by Euler angles.
		m.scale(Vector3(-1, -1, -1));
	}

	return m.get_quat();
}

void Basis::get_rotation_axis_angle(Vector3& p_axis, double& p_angle) const {
	// Assumes that the matrix can be decomposed into a proper rotation and scaling matrix as M = R.S,
	// and returns the Euler angles corresponding to the rotation part, complementing get_scale().
	// See the comment in get_scale() for further information.
	Basis m = orthonormalized();
	double det = m.determinant();
	if (det < 0) {
		// Ensure that the determinant is 1, such that result is a proper rotation matrix which can be represented by Euler angles.
		m.scale(Vector3(-1, -1, -1));
	}

	m.get_axis_angle(p_axis, p_angle);
}

void Basis::get_rotation_axis_angle_local(Vector3& p_axis, double& p_angle) const {
	// Assumes that the matrix can be decomposed into a proper rotation and scaling matrix as M = R.S,
	// and returns the Euler angles corresponding to the rotation part, complementing get_scale().
	// See the comment in get_scale() for further information.
	Basis m = transposed();
	m.orthonormalize();
	double det = m.determinant();
	if (det < 0) {
		// Ensure that the determinant is 1, such that result is a proper rotation matrix which can be represented by Euler angles.
		m.scale(Vector3(-1, -1, -1));
	}

	m.get_axis_angle(p_axis, p_angle);
	p_angle = -p_angle;
}

// get_euler_xyz returns a vector containing the Euler angles in the format
// (a1,a2,a3), where a3 is the angle of the first rotation, and a1 is the last
// (following the convention they are commonly defined in the literature).
//
// The current implementation uses XYZ convention (Z is the first rotation),
// so euler.z is the angle of the (first) rotation around Z axis and so on,
//
// And thus, assuming the matrix is a rotation matrix, this function returns
// the angles in the decomposition R = X(a1).Y(a2).Z(a3) where Z(a) rotates
// around the z-axis by a and so on.
Vector3 Basis::get_euler_xyz() const {
	// Euler angles in XYZ convention.
	// See https://en.wikipedia.org/wiki/Euler_angles#Rotation_matrix
	//
	// rot =  cy*cz          -cy*sz           sy
	//        cz*sx*sy+cx*sz  cx*cz-sx*sy*sz -cy*sx
	//       -cx*cz*sy+sx*sz  cz*sx+cx*sy*sz  cx*cy

	Vector3 euler;
	double sy = elements[0][2];
	if (sy < (1.0 - UNIT_EPSILON)) {
		if (sy > -(1.0 - UNIT_EPSILON)) {
			// is this a pure Y rotation?
			if (elements[1][0] == 0.0 && elements[0][1] == 0.0 && elements[1][2] == 0 && elements[2][1] == 0 && elements[1][1] == 1) {
				// return the simplest form (human friendlier in editor and scripts)
				euler.x = 0;
				euler.y = atan2(elements[0][2], elements[0][0]);
				euler.z = 0;
			}
			else {
				euler.x = std::atan2(-elements[1][2], elements[2][2]);
				euler.y = std::asin(sy);
				euler.z = std::atan2(-elements[0][1], elements[0][0]);
			}
		}
		else {
			euler.x = std::atan2(elements[2][1], elements[1][1]);
			euler.y = -Math_PI / 2.0;
			euler.z = 0.0;
		}
	}
	else {
		euler.x = std::atan2(elements[2][1], elements[1][1]);
		euler.y = Math_PI / 2.0;
		euler.z = 0.0;
	}
	return euler;
}

// set_euler_xyz expects a vector containing the Euler angles in the format
// (ax,ay,az), where ax is the angle of rotation around x axis,
// and similar for other axes.
// The current implementation uses XYZ convention (Z is the first rotation).
void Basis::set_euler_xyz(const Vector3& p_euler) {
	double c, s;

	c = std::cos(p_euler.x);
	s = std::sin(p_euler.x);
	Basis xmat(1.0, 0.0, 0.0, 0.0, c, -s, 0.0, s, c);

	c = std::cos(p_euler.y);
	s = std::sin(p_euler.y);
	Basis ymat(c, 0.0, s, 0.0, 1.0, 0.0, -s, 0.0, c);

	c = std::cos(p_euler.z);
	s = std::sin(p_euler.z);
	Basis zmat(c, -s, 0.0, s, c, 0.0, 0.0, 0.0, 1.0);

	//optimizer will optimize away all this anyway
	*this = xmat * (ymat * zmat);
}

Vector3 Basis::get_euler_xzy() const {
	// Euler angles in XZY convention.
	// See https://en.wikipedia.org/wiki/Euler_angles#Rotation_matrix
	//
	// rot =  cz*cy             -sz             cz*sy
	//        sx*sy+cx*cy*sz    cx*cz           cx*sz*sy-cy*sx
	//        cy*sx*sz          cz*sx           cx*cy+sx*sz*sy

	Vector3 euler;
	double sz = elements[0][1];
	if (sz < (1.0 - UNIT_EPSILON)) {
		if (sz > -(1.0 - UNIT_EPSILON)) {
			euler.x = std::atan2(elements[2][1], elements[1][1]);
			euler.y = std::atan2(elements[0][2], elements[0][0]);
			euler.z = std::asin(-sz);
		}
		else {
			// It's -1
			euler.x = -std::atan2(elements[1][2], elements[2][2]);
			euler.y = 0.0;
			euler.z = Math_PI / 2.0;
		}
	}
	else {
		// It's 1
		euler.x = -std::atan2(elements[1][2], elements[2][2]);
		euler.y = 0.0;
		euler.z = -Math_PI / 2.0;
	}
	return euler;
}

void Basis::set_euler_xzy(const Vector3& p_euler) {
	double c, s;

	c = std::cos(p_euler.x);
	s = std::sin(p_euler.x);
	Basis xmat(1.0, 0.0, 0.0, 0.0, c, -s, 0.0, s, c);

	c = std::cos(p_euler.y);
	s = std::sin(p_euler.y);
	Basis ymat(c, 0.0, s, 0.0, 1.0, 0.0, -s, 0.0, c);

	c = std::cos(p_euler.z);
	s = std::sin(p_euler.z);
	Basis zmat(c, -s, 0.0, s, c, 0.0, 0.0, 0.0, 1.0);

	*this = xmat * zmat * ymat;
}

Vector3 Basis::get_euler_yzx() const {
	// Euler angles in YZX convention.
	// See https://en.wikipedia.org/wiki/Euler_angles#Rotation_matrix
	//
	// rot =  cy*cz             sy*sx-cy*cx*sz     cx*sy+cy*sz*sx
	//        sz                cz*cx              -cz*sx
	//        -cz*sy            cy*sx+cx*sy*sz     cy*cx-sy*sz*sx

	Vector3 euler;
	double sz = elements[1][0];
	if (sz < (1.0 - UNIT_EPSILON)) {
		if (sz > -(1.0 - UNIT_EPSILON)) {
			euler.x = std::atan2(-elements[1][2], elements[1][1]);
			euler.y = std::atan2(-elements[2][0], elements[0][0]);
			euler.z = std::asin(sz);
		}
		else {
			// It's -1
			euler.x = std::atan2(elements[2][1], elements[2][2]);
			euler.y = 0.0;
			euler.z = -Math_PI / 2.0;
		}
	}
	else {
		// It's 1
		euler.x = std::atan2(elements[2][1], elements[2][2]);
		euler.y = 0.0;
		euler.z = Math_PI / 2.0;
	}
	return euler;
}

void Basis::set_euler_yzx(const Vector3& p_euler) {
	double c, s;

	c = std::cos(p_euler.x);
	s = std::sin(p_euler.x);
	Basis xmat(1.0, 0.0, 0.0, 0.0, c, -s, 0.0, s, c);

	c = std::cos(p_euler.y);
	s = std::sin(p_euler.y);
	Basis ymat(c, 0.0, s, 0.0, 1.0, 0.0, -s, 0.0, c);

	c = std::cos(p_euler.z);
	s = std::sin(p_euler.z);
	Basis zmat(c, -s, 0.0, s, c, 0.0, 0.0, 0.0, 1.0);

	*this = ymat * zmat * xmat;
}

// get_euler_yxz returns a vector containing the Euler angles in the YXZ convention,
// as in first-Z, then-X, last-Y. The angles for X, Y, and Z rotations are returned
// as the x, y, and z components of a Vector3 respectively.
Vector3 Basis::get_euler_yxz() const {
	// Euler angles in YXZ convention.
	// See https://en.wikipedia.org/wiki/Euler_angles#Rotation_matrix
	//
	// rot =  cy*cz+sy*sx*sz    cz*sy*sx-cy*sz        cx*sy
	//        cx*sz             cx*cz                 -sx
	//        cy*sx*sz-cz*sy    cy*cz*sx+sy*sz        cy*cx

	Vector3 euler;

	double m12 = elements[1][2];

	if (m12 < (1 - UNIT_EPSILON)) {
		if (m12 > -(1 - UNIT_EPSILON)) {
			// is this a pure X rotation?
			if (elements[1][0] == 0 && elements[0][1] == 0 && elements[0][2] == 0 && elements[2][0] == 0 && elements[0][0] == 1) {
				// return the simplest form (human friendlier in editor and scripts)
				euler.x = atan2(-m12, elements[1][1]);
				euler.y = 0;
				euler.z = 0;
			}
			else {
				euler.x = asin(-m12);
				euler.y = atan2(elements[0][2], elements[2][2]);
				euler.z = atan2(elements[1][0], elements[1][1]);
			}
		}
		else { // m12 == -1
			euler.x = Math_PI * 0.5;
			euler.y = atan2(elements[0][1], elements[0][0]);
			euler.z = 0;
		}
	}
	else { // m12 == 1
		euler.x = -Math_PI * 0.5;
		euler.y = -atan2(elements[0][1], elements[0][0]);
		euler.z = 0;
	}

	return euler;
}

// set_euler_yxz expects a vector containing the Euler angles in the format
// (ax,ay,az), where ax is the angle of rotation around x axis,
// and similar for other axes.
// The current implementation uses YXZ convention (Z is the first rotation).
void Basis::set_euler_yxz(const Vector3& p_euler) {
	double c, s;

	c = std::cos(p_euler.x);
	s = std::sin(p_euler.x);
	Basis xmat(1.0, 0.0, 0.0, 0.0, c, -s, 0.0, s, c);

	c = std::cos(p_euler.y);
	s = std::sin(p_euler.y);
	Basis ymat(c, 0.0, s, 0.0, 1.0, 0.0, -s, 0.0, c);

	c = std::cos(p_euler.z);
	s = std::sin(p_euler.z);
	Basis zmat(c, -s, 0.0, s, c, 0.0, 0.0, 0.0, 1.0);

	//optimizer will optimize away all this anyway
	*this = ymat * xmat * zmat;
}

Vector3 Basis::get_euler_zxy() const {
	// Euler angles in ZXY convention.
	// See https://en.wikipedia.org/wiki/Euler_angles#Rotation_matrix
	//
	// rot =  cz*cy-sz*sx*sy    -cx*sz                cz*sy+cy*sz*sx
	//        cy*sz+cz*sx*sy    cz*cx                 sz*sy-cz*cy*sx
	//        -cx*sy            sx                    cx*cy
	Vector3 euler;
	double sx = elements[2][1];
	if (sx < (1.0 - UNIT_EPSILON)) {
		if (sx > -(1.0 - UNIT_EPSILON)) {
			euler.x = std::asin(sx);
			euler.y = std::atan2(-elements[2][0], elements[2][2]);
			euler.z = std::atan2(-elements[0][1], elements[1][1]);
		}
		else {
			// It's -1
			euler.x = -Math_PI / 2.0;
			euler.y = std::atan2(elements[0][2], elements[0][0]);
			euler.z = 0;
		}
	}
	else {
		// It's 1
		euler.x = Math_PI / 2.0;
		euler.y = std::atan2(elements[0][2], elements[0][0]);
		euler.z = 0;
	}
	return euler;
}

void Basis::set_euler_zxy(const Vector3& p_euler) {
	double c, s;

	c = std::cos(p_euler.x);
	s = std::sin(p_euler.x);
	Basis xmat(1.0, 0.0, 0.0, 0.0, c, -s, 0.0, s, c);

	c = std::cos(p_euler.y);
	s = std::sin(p_euler.y);
	Basis ymat(c, 0.0, s, 0.0, 1.0, 0.0, -s, 0.0, c);

	c = std::cos(p_euler.z);
	s = std::sin(p_euler.z);
	Basis zmat(c, -s, 0.0, s, c, 0.0, 0.0, 0.0, 1.0);

	*this = zmat * xmat * ymat;
}

Vector3 Basis::get_euler_zyx() const {
	// Euler angles in ZYX convention.
	// See https://en.wikipedia.org/wiki/Euler_angles#Rotation_matrix
	//
	// rot =  cz*cy             cz*sy*sx-cx*sz        sz*sx+cz*cx*cy
	//        cy*sz             cz*cx+sz*sy*sx        cx*sz*sy-cz*sx
	//        -sy               cy*sx                 cy*cx
	Vector3 euler;
	double sy = elements[2][0];
	if (sy < (1.0 - UNIT_EPSILON)) {
		if (sy > -(1.0 - UNIT_EPSILON)) {
			euler.x = std::atan2(elements[2][1], elements[2][2]);
			euler.y = std::asin(-sy);
			euler.z = std::atan2(elements[1][0], elements[0][0]);
		}
		else {
			// It's -1
			euler.x = 0;
			euler.y = Math_PI / 2.0;
			euler.z = -std::atan2(elements[0][1], elements[1][1]);
		}
	}
	else {
		// It's 1
		euler.x = 0;
		euler.y = -Math_PI / 2.0;
		euler.z = -std::atan2(elements[0][1], elements[1][1]);
	}
	return euler;
}

void Basis::set_euler_zyx(const Vector3& p_euler) {
	double c, s;

	c = std::cos(p_euler.x);
	s = std::sin(p_euler.x);
	Basis xmat(1.0, 0.0, 0.0, 0.0, c, -s, 0.0, s, c);

	c = std::cos(p_euler.y);
	s = std::sin(p_euler.y);
	Basis ymat(c, 0.0, s, 0.0, 1.0, 0.0, -s, 0.0, c);

	c = std::cos(p_euler.z);
	s = std::sin(p_euler.z);
	Basis zmat(c, -s, 0.0, s, c, 0.0, 0.0, 0.0, 1.0);

	*this = zmat * ymat * xmat;
}

bool Basis::is_equal_approx(const Basis& p_basis) const {
	return elements[0].is_equal_approx(p_basis.elements[0]) && elements[1].is_equal_approx(p_basis.elements[1]) && elements[2].is_equal_approx(p_basis.elements[2]);
}

bool Basis::operator==(const Basis& p_matrix) const {
	for (int i = 0; i < 3; i++) {
		for (int j = 0; j < 3; j++) {
			if (elements[i][j] != p_matrix.elements[i][j]) {
				return false;
			}
		}
	}

	return true;
}

bool Basis::operator!=(const Basis& p_matrix) const {
	return (!(*this == p_matrix));
}


Quat Basis::get_quat() const {
#ifdef MATH_CHECKS
	ERR_FAIL_COND_V_MSG(!is_rotation(), Quat(), "Basis must be normalized in order to be casted to a Quaternion. Use get_rotation_quat() or call orthonormalized() instead.");
#endif
	/* Allow getting a quaternion from an unnormalized transform */
	Basis m = *this;
	double trace = m.elements[0][0] + m.elements[1][1] + m.elements[2][2];
	double temp[4];

	if (trace > 0.0) {
		double s = std::sqrt(trace + 1.0);
		temp[3] = (s * 0.5);
		s = 0.5 / s;

		temp[0] = ((m.elements[2][1] - m.elements[1][2]) * s);
		temp[1] = ((m.elements[0][2] - m.elements[2][0]) * s);
		temp[2] = ((m.elements[1][0] - m.elements[0][1]) * s);
	}
	else {
		int i = m.elements[0][0] < m.elements[1][1] ?
			(m.elements[1][1] < m.elements[2][2] ? 2 : 1) :
			(m.elements[0][0] < m.elements[2][2] ? 2 : 0);
		int j = (i + 1) % 3;
		int k = (i + 2) % 3;

		double s = std::sqrt(m.elements[i][i] - m.elements[j][j] - m.elements[k][k] + 1.0);
		temp[i] = s * 0.5;
		s = 0.5 / s;

		temp[3] = (m.elements[k][j] - m.elements[j][k]) * s;
		temp[j] = (m.elements[j][i] + m.elements[i][j]) * s;
		temp[k] = (m.elements[k][i] + m.elements[i][k]) * s;
	}

	return Quat(temp[0], temp[1], temp[2], temp[3]);
}

static const Basis _ortho_bases[24] = {
	Basis(1, 0, 0, 0, 1, 0, 0, 0, 1),
	Basis(0, -1, 0, 1, 0, 0, 0, 0, 1),
	Basis(-1, 0, 0, 0, -1, 0, 0, 0, 1),
	Basis(0, 1, 0, -1, 0, 0, 0, 0, 1),
	Basis(1, 0, 0, 0, 0, -1, 0, 1, 0),
	Basis(0, 0, 1, 1, 0, 0, 0, 1, 0),
	Basis(-1, 0, 0, 0, 0, 1, 0, 1, 0),
	Basis(0, 0, -1, -1, 0, 0, 0, 1, 0),
	Basis(1, 0, 0, 0, -1, 0, 0, 0, -1),
	Basis(0, 1, 0, 1, 0, 0, 0, 0, -1),
	Basis(-1, 0, 0, 0, 1, 0, 0, 0, -1),
	Basis(0, -1, 0, -1, 0, 0, 0, 0, -1),
	Basis(1, 0, 0, 0, 0, 1, 0, -1, 0),
	Basis(0, 0, -1, 1, 0, 0, 0, -1, 0),
	Basis(-1, 0, 0, 0, 0, -1, 0, -1, 0),
	Basis(0, 0, 1, -1, 0, 0, 0, -1, 0),
	Basis(0, 0, 1, 0, 1, 0, -1, 0, 0),
	Basis(0, -1, 0, 0, 0, 1, -1, 0, 0),
	Basis(0, 0, -1, 0, -1, 0, -1, 0, 0),
	Basis(0, 1, 0, 0, 0, -1, -1, 0, 0),
	Basis(0, 0, 1, 0, -1, 0, 1, 0, 0),
	Basis(0, 1, 0, 0, 0, 1, 1, 0, 0),
	Basis(0, 0, -1, 0, 1, 0, 1, 0, 0),
	Basis(0, -1, 0, 0, 0, -1, 1, 0, 0)
};

int Basis::get_orthogonal_index() const {
	//could be sped up if i come up with a way
	Basis orth = *this;
	for (int i = 0; i < 3; i++) {
		for (int j = 0; j < 3; j++) {
			double v = orth[i][j];
			if (v > 0.5) {
				v = 1.0;
			}
			else if (v < -0.5) {
				v = -1.0;
			}
			else {
				v = 0;
			}

			orth[i][j] = v;
		}
	}

	for (int i = 0; i < 24; i++) {
		if (_ortho_bases[i] == orth) {
			return i;
		}
	}

	return 0;
}

void Basis::set_orthogonal_index(int p_index) {
	//there only exist 24 orthogonal bases in r3

	*this = _ortho_bases[p_index];
}

void Basis::get_axis_angle(Vector3& r_axis, double& r_angle) const {
	/* checking this is a bad idea, because obtaining from scaled transform is a valid use case
#ifdef MATH_CHECKS
	ERR_FAIL_COND(!is_rotation());
#endif
*/
	double angle, x, y, z; // variables for result
	double epsilon = 0.01; // margin to allow for rounding errors
	double epsilon2 = 0.1; // margin to distinguish between 0 and 180 degrees

	if ((std::abs(elements[1][0] - elements[0][1]) < epsilon) && (std::abs(elements[2][0] - elements[0][2]) < epsilon) && (std::abs(elements[2][1] - elements[1][2]) < epsilon)) {
		// singularity found
		// first check for identity matrix which must have +1 for all terms
		//  in leading diagonaland zero in other terms
		if ((std::abs(elements[1][0] + elements[0][1]) < epsilon2) && (std::abs(elements[2][0] + elements[0][2]) < epsilon2) && (std::abs(elements[2][1] + elements[1][2]) < epsilon2) && (std::abs(elements[0][0] + elements[1][1] + elements[2][2] - 3) < epsilon2)) {
			// this singularity is identity matrix so angle = 0
			r_axis = Vector3(0, 1, 0);
			r_angle = 0;
			return;
		}
		// otherwise this singularity is angle = 180
		angle = Math_PI;
		double xx = (elements[0][0] + 1) / 2;
		double yy = (elements[1][1] + 1) / 2;
		double zz = (elements[2][2] + 1) / 2;
		double xy = (elements[1][0] + elements[0][1]) / 4;
		double xz = (elements[2][0] + elements[0][2]) / 4;
		double yz = (elements[2][1] + elements[1][2]) / 4;
		if ((xx > yy) && (xx > zz)) { // elements[0][0] is the largest diagonal term
			if (xx < epsilon) {
				x = 0;
				y = Math_SQRT12;
				z = Math_SQRT12;
			}
			else {
				x = std::sqrt(xx);
				y = xy / x;
				z = xz / x;
			}
		}
		else if (yy > zz) { // elements[1][1] is the largest diagonal term
			if (yy < epsilon) {
				x = Math_SQRT12;
				y = 0;
				z = Math_SQRT12;
			}
			else {
				y = std::sqrt(yy);
				x = xy / y;
				z = yz / y;
			}
		}
		else { // elements[2][2] is the largest diagonal term so base result on this
			if (zz < epsilon) {
				x = Math_SQRT12;
				y = Math_SQRT12;
				z = 0;
			}
			else {
				z = std::sqrt(zz);
				x = xz / z;
				y = yz / z;
			}
		}
		r_axis = Vector3(x, y, z);
		r_angle = angle;
		return;
	}
	// as we have reached here there are no singularities so we can handle normally
	double s = std::sqrt((elements[1][2] - elements[2][1]) * (elements[1][2] - elements[2][1]) + (elements[2][0] - elements[0][2]) * (elements[2][0] - elements[0][2]) + (elements[0][1] - elements[1][0]) * (elements[0][1] - elements[1][0])); // s=|axis||sin(angle)|, used to normalise

	angle = std::acos((elements[0][0] + elements[1][1] + elements[2][2] - 1) / 2);
	if (angle < 0) {
		s = -s;
	}
	x = (elements[2][1] - elements[1][2]) / s;
	y = (elements[0][2] - elements[2][0]) / s;
	z = (elements[1][0] - elements[0][1]) / s;

	r_axis = Vector3(x, y, z);
	r_angle = angle;
}

void Basis::set_quat(const Quat& p_quat) {
	double d = p_quat.length_squared();
	double s = 2.0 / d;
	double xs = p_quat.x * s, ys = p_quat.y * s, zs = p_quat.z * s;
	double wx = p_quat.w * xs, wy = p_quat.w * ys, wz = p_quat.w * zs;
	double xx = p_quat.x * xs, xy = p_quat.x * ys, xz = p_quat.x * zs;
	double yy = p_quat.y * ys, yz = p_quat.y * zs, zz = p_quat.z * zs;
	set(1.0 - (yy + zz), xy - wz, xz + wy,
		xy + wz, 1.0 - (xx + zz), yz - wx,
		xz - wy, yz + wx, 1.0 - (xx + yy));
}

void Basis::set_axis_angle(const Vector3& p_axis, double p_phi) {
	// Rotation matrix from axis and angle, see https://en.wikipedia.org/wiki/Rotation_matrix#Rotation_matrix_from_axis_angle
#ifdef MATH_CHECKS
	ERR_FAIL_COND_MSG(!p_axis.is_normalized(), "The axis Vector3 must be normalized.");
#endif
	Vector3 axis_sq(p_axis.x * p_axis.x, p_axis.y * p_axis.y, p_axis.z * p_axis.z);
	double cosine = std::cos(p_phi);
	elements[0][0] = axis_sq.x + cosine * (1.0 - axis_sq.x);
	elements[1][1] = axis_sq.y + cosine * (1.0 - axis_sq.y);
	elements[2][2] = axis_sq.z + cosine * (1.0 - axis_sq.z);

	double sine = std::sin(p_phi);
	double t = 1 - cosine;

	double xyzt = p_axis.x * p_axis.y * t;
	double zyxs = p_axis.z * sine;
	elements[0][1] = xyzt - zyxs;
	elements[1][0] = xyzt + zyxs;

	xyzt = p_axis.x * p_axis.z * t;
	zyxs = p_axis.y * sine;
	elements[0][2] = xyzt + zyxs;
	elements[2][0] = xyzt - zyxs;

	xyzt = p_axis.y * p_axis.z * t;
	zyxs = p_axis.x * sine;
	elements[1][2] = xyzt - zyxs;
	elements[2][1] = xyzt + zyxs;
}

void Basis::set_axis_angle_scale(const Vector3& p_axis, double p_phi, const Vector3& p_scale) {
	set_diagonal(p_scale);
	rotate(p_axis, p_phi);
}

void Basis::set_euler_scale(const Vector3& p_euler, const Vector3& p_scale) {
	set_diagonal(p_scale);
	rotate(p_euler);
}

void Basis::set_quat_scale(const Quat& p_quat, const Vector3& p_scale) {
	set_diagonal(p_scale);
	rotate(p_quat);
}

void Basis::set_diagonal(const Vector3& p_diag) {
	elements[0][0] = p_diag.x;
	elements[0][1] = 0;
	elements[0][2] = 0;

	elements[1][0] = 0;
	elements[1][1] = p_diag.y;
	elements[1][2] = 0;

	elements[2][0] = 0;
	elements[2][1] = 0;
	elements[2][2] = p_diag.z;
}

Basis Basis::slerp(const Basis& target, const double& t) const {
	//consider scale
	Quat from(*this);
	Quat to(target);

	Basis b(from.slerp(to, t));
	b.elements[0] *= Math::lerp(elements[0].length(), target.elements[0].length(), t);
	b.elements[1] *= Math::lerp(elements[1].length(), target.elements[1].length(), t);
	b.elements[2] *= Math::lerp(elements[2].length(), target.elements[2].length(), t);

	return b;
}

void Basis::rotate_sh(double* p_values) {
	// code by John Hable
	// http://filmicworlds.com/blog/simple-and-fast-spherical-harmonic-rotation/
	// this code is Public Domain

	const static double s_c3 = 0.94617469575; // (3*sqrt(5))/(4*sqrt(pi))
	const static double s_c4 = -0.31539156525; // (-sqrt(5))/(4*sqrt(pi))
	const static double s_c5 = 0.54627421529; // (sqrt(15))/(4*sqrt(pi))

	const static double s_c_scale = 1.0 / 0.91529123286551084;
	const static double s_c_scale_inv = 0.91529123286551084;

	const static double s_rc2 = 1.5853309190550713 * s_c_scale;
	const static double s_c4_div_c3 = s_c4 / s_c3;
	const static double s_c4_div_c3_x2 = (s_c4 / s_c3) * 2.0;

	const static double s_scale_dst2 = s_c3 * s_c_scale_inv;
	const static double s_scale_dst4 = s_c5 * s_c_scale_inv;

	double src[9] = { p_values[0], p_values[1], p_values[2], p_values[3], p_values[4], p_values[5], p_values[6], p_values[7], p_values[8] };

	double m00 = elements[0][0];
	double m01 = elements[0][1];
	double m02 = elements[0][2];
	double m10 = elements[1][0];
	double m11 = elements[1][1];
	double m12 = elements[1][2];
	double m20 = elements[2][0];
	double m21 = elements[2][1];
	double m22 = elements[2][2];

	p_values[0] = src[0];
	p_values[1] = m11 * src[1] - m12 * src[2] + m10 * src[3];
	p_values[2] = -m21 * src[1] + m22 * src[2] - m20 * src[3];
	p_values[3] = m01 * src[1] - m02 * src[2] + m00 * src[3];

	double sh0 = src[7] + src[8] + src[8] - src[5];
	double sh1 = src[4] + s_rc2 * src[6] + src[7] + src[8];
	double sh2 = src[4];
	double sh3 = -src[7];
	double sh4 = -src[5];

	// Rotations.  R0 and R1 just use the raw matrix columns
	double r2x = m00 + m01;
	double r2y = m10 + m11;
	double r2z = m20 + m21;

	double r3x = m00 + m02;
	double r3y = m10 + m12;
	double r3z = m20 + m22;

	double r4x = m01 + m02;
	double r4y = m11 + m12;
	double r4z = m21 + m22;

	// dense matrix multiplication one column at a time

	// column 0
	double sh0_x = sh0 * m00;
	double sh0_y = sh0 * m10;
	double d0 = sh0_x * m10;
	double d1 = sh0_y * m20;
	double d2 = sh0 * (m20 * m20 + s_c4_div_c3);
	double d3 = sh0_x * m20;
	double d4 = sh0_x * m00 - sh0_y * m10;

	// column 1
	double sh1_x = sh1 * m02;
	double sh1_y = sh1 * m12;
	d0 += sh1_x * m12;
	d1 += sh1_y * m22;
	d2 += sh1 * (m22 * m22 + s_c4_div_c3);
	d3 += sh1_x * m22;
	d4 += sh1_x * m02 - sh1_y * m12;

	// column 2
	double sh2_x = sh2 * r2x;
	double sh2_y = sh2 * r2y;
	d0 += sh2_x * r2y;
	d1 += sh2_y * r2z;
	d2 += sh2 * (r2z * r2z + s_c4_div_c3_x2);
	d3 += sh2_x * r2z;
	d4 += sh2_x * r2x - sh2_y * r2y;

	// column 3
	double sh3_x = sh3 * r3x;
	double sh3_y = sh3 * r3y;
	d0 += sh3_x * r3y;
	d1 += sh3_y * r3z;
	d2 += sh3 * (r3z * r3z + s_c4_div_c3_x2);
	d3 += sh3_x * r3z;
	d4 += sh3_x * r3x - sh3_y * r3y;

	// column 4
	double sh4_x = sh4 * r4x;
	double sh4_y = sh4 * r4y;
	d0 += sh4_x * r4y;
	d1 += sh4_y * r4z;
	d2 += sh4 * (r4z * r4z + s_c4_div_c3_x2);
	d3 += sh4_x * r4z;
	d4 += sh4_x * r4x - sh4_y * r4y;

	// extra multipliers
	p_values[4] = d0;
	p_values[5] = -d1;
	p_values[6] = d2 * s_scale_dst2;
	p_values[7] = -d3;
	p_values[8] = d4 * s_scale_dst4;
}
