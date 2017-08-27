
#ifndef CBMATH_H
#define CBMATH_H

#include <math.h>

#ifndef _WIN32
#include <Accelerate/Accelerate.h>
#endif

#ifdef _WIN32
// TEMPORARY reference implementation, may be replaced by SSE intrinsics if we have time.
class V4
{
private:
	float x, y, z, w;
public:
	V4 operator+(const V4& vec) const
	{
		V4 result;
		result.x = x + vec.x;
		result.y = y + vec.y;
		result.z = z + vec.z;
		result.w = w + vec.w;
		return result;
	}
	V4 operator-(const V4& vec) const
	{
		V4 result;
		result.x = x - vec.x;
		result.y = y - vec.y;
		result.z = z - vec.z;
		result.w = w - vec.w;
		return result;
	}
	V4& operator+=(V4& vec)
	{
		x += vec.x;
		y += vec.y;
		z += vec.z;
		w += vec.w;
		return *this;
	}
	V4& operator-=(V4& vec)
	{
		x -= vec.x;
		y -= vec.y;
		z -= vec.z;
		w -= vec.w;
		return *this;
	}
};
#else
typedef VectorFloat V4;
#endif

union Vec4
{
	V4 v4;			// use this form to access vector intrinsic functions (and built-in operators)
	float f[4];
	struct {
		float x;
		float y;
		float z;
		float w;
	};

	Vec4 operator+(const Vec4& vec) const
	{
		Vec4 result;
		result.v4 = v4 + vec.v4;
		return result;
	}
	Vec4 operator-(const Vec4& vec) const
	{
		Vec4 result;
		result.v4 = v4 - vec.v4;
		return result;
	}

	Vec4 operator*(const float f) const
	{
		Vec4 result;
		result.x = x * f;
		result.y = y * f;
		result.z = z * f;
		result.w = w * f;
		return result;
	}

	Vec4& operator+=(Vec4& vec)
	{
		x += vec.x;
		y += vec.y;
		z += vec.z;
		w += vec.w;
//		v4 += vec.v4;
		return *this;
	}
	Vec4& operator-=(Vec4& vec)
	{
		v4 -= vec.v4;
		return *this;
	}
};

typedef Vec4 Vec3;
typedef Vec4 Vec2;

union Mat44
{
	Vec4 v[4];
	float f[16];
	struct {
		Vec4 x;
		Vec4 y;
		Vec4 z;
		Vec4 t;
	};
};

inline void Vec4Set(Vec4 &d, float x, float y, float z, float w)
{
	d.x = x;
	d.y = y;
	d.z = z;
	d.w = w;
}

inline void Vec3Set(Vec3 &d, float x, float y, float z)
{
	d.x = x;
	d.y = y;
	d.z = z;
	d.w = 0.0f;
}

inline void Vec2Set(Vec2 &d, float x, float y)
{
	d.x = x;
	d.y = y;
	d.z = 0.0f;
	d.w = 0.0f;
}

inline void Vec3Cross(Vec3 &dst, const Vec3 &a, const Vec3 &b)
{
	float dx = a.y * b.z - a.z * b.y;
	float dy = a.z * b.x - a.x * b.z;
	float dz = a.x * b.y - a.y * b.x;
	dst.x = dx;
	dst.y = dy;
	dst.z = dz;
}

inline float Vec3Dot(const Vec3 a, const Vec3 b)
{
	return a.x * b.x + a.y * b.y + a.z * b.z;
}

inline float Vec3LengthSqr(const Vec3 a)
{
	return Vec3Dot(a, a);
}

inline float Vec3Length(const Vec3 a)
{
	float lengthSqr = Vec3LengthSqr(a);
	if (lengthSqr < 0.000001f)
		return 0.0f;
	else
		return sqrtf(lengthSqr);
}

inline void Vec3Normalize(Vec3 &dst, const Vec3 src)
{
	float length = Vec3Length(src);
	if (length < 0.000001f)
	{
		// Need to add an assert mechanism for this
		return;
	}
	else
	{
		float recipLength = 1.0f / length;
		dst.x = src.x * recipLength;
		dst.y = src.y * recipLength;
		dst.z = src.z * recipLength;
	}
}

inline void Mat44MakeIdentity(Mat44 &m)
{
	Vec4Set(m.x, 1.0f, 0.0f, 0.0f, 0.0f);
	Vec4Set(m.y, 0.0f, 1.0f, 0.0f, 0.0f);
	Vec4Set(m.z, 0.0f, 0.0f, 1.0f, 0.0f);
	Vec4Set(m.t, 0.0f, 0.0f, 0.0f, 1.0f);
}

// Some Random utilities which should be moved out of here.
#define		RANDOMINT (((int32)rand()) | (((int32)rand()) << 16))
#define		RANDOM0TO1	((RANDOMINT / 2147483648.0f))
#define		RANDOMPLUSMINUS	((RANDOMINT / 1073741824.0f) - 1.0f)
#define		RANDOM_IN_RANGE(_min, _max)	((_min) + (((_max) - (_min)) * RANDOM0TO1))


#endif // CBMATH_H