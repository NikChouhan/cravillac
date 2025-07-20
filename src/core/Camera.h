#pragma once

#include <assert.h>
#include <algorithm>
#include <SimpleMath.h>

#undef min

class CameraPositionerInterface
{
public:
	virtual ~CameraPositionerInterface() = default;
	virtual SM::Matrix getViewMatrix() const = 0;
	virtual SM::Vector3 getPosition() const = 0;
};

class Camera final
{
public:
	explicit Camera(CameraPositionerInterface& positioner)
		: positioner_(&positioner)
	{}

	Camera(const Camera&) = default;
	Camera& operator = (const Camera&) = default;

	SM::Matrix getViewMatrix() const { return positioner_->getViewMatrix(); }
	SM::Vector3 getPosition() const { return positioner_->getPosition(); }
	DirectX::XMMATRIX getProjMatrix() const { return proj_; }
	void InitPerspective()
	{
		proj_ = DirectX::XMMatrixPerspectiveFovRH(DirectX::XMConvertToRadians(45.f), 16./9., 0.1f, 1000.0f);
		proj_.r[1].m128_f32[1] *= -1.0f;
	}

private:
	const CameraPositionerInterface* positioner_;
	DirectX::XMMATRIX proj_;
};

class CameraPositioner_FirstPerson final: public CameraPositionerInterface
{
public:
	CameraPositioner_FirstPerson() = default;
	CameraPositioner_FirstPerson(const SM::Vector3& pos, const SM::Vector3& target, const SM::Vector3& up)
	: cameraPosition_(pos)
	, cameraOrientation_(DirectX::XMQuaternionRotationMatrix(DirectX::XMMatrixLookAtRH(pos, target, up)))
	, up_(up)
	{}

	void update(double deltaSeconds, const SM::Vector2& mousePos, bool mousePressed)
	{
		if (mousePressed)
		{
			const SM::Vector2 delta = mousePos - mousePos_;
			const SM::Quaternion deltaQuat = SM::Quaternion(SM::Vector3(-mouseSpeed_ * delta.y, mouseSpeed_ * delta.x, 0.0f));
			cameraOrientation_ = deltaQuat * cameraOrientation_;
			cameraOrientation_.Normalize();
			setUpVector(up_);
		}
		mousePos_ = mousePos;

		const SM::Matrix v = DirectX::XMMatrixRotationQuaternion(cameraOrientation_);

		SM::Vector3 forward = DirectX::XMVectorNegate(SM::Vector3(v._13, v._23, v._33));
		SM::Vector3 right = SM::Vector3(v._11, v._21, v._31);
		const SM::Vector3 up = DirectX::XMVector3Cross(right, forward);

		SM::Vector3 accel(0.0f);

		if (movement_.forward_) accel += forward;
		if (movement_.backward_) accel -= forward;

		if (movement_.left_) accel -= right;
		if (movement_.right_) accel += right;

		if (movement_.up_) accel += up;
		if (movement_.down_) accel -= up;

		if (movement_.fastSpeed_) accel *= fastCoef_;

		if (accel == SM::Vector3{ 0.,0.,0. })
		{
			// decelerate naturally according to the damping value
			moveSpeed_ -= moveSpeed_ * std::min((1.0f / damping_) * static_cast<float>(deltaSeconds), 1.0f);
		}
		else
		{
			// acceleration
			moveSpeed_ += accel * acceleration_ * static_cast<float>(deltaSeconds);
			const float maxSpeed = movement_.fastSpeed_ ? maxSpeed_ * fastCoef_ : maxSpeed_;
			if ((moveSpeed_.Length()) > maxSpeed)
			{
				moveSpeed_.Normalize();
				moveSpeed_ = DirectX::XMVectorScale((moveSpeed_), maxSpeed);
			}
		}

		cameraPosition_ += moveSpeed_ * static_cast<float>(deltaSeconds);
	}

	virtual SM::Matrix getViewMatrix() const override
	{
		SM::Matrix t = SM::Matrix::CreateTranslation(-cameraPosition_);
		const SM::Matrix r = SM::Matrix::CreateFromQuaternion(cameraOrientation_);
		return r * t;
	}

	virtual SM::Vector3 getPosition() const override
	{
		return cameraPosition_;
	}

	void setPosition(const SM::Vector3& pos)
	{
		cameraPosition_ = pos;
	}

	void setSpeed(const SM::Vector3& speed) {
		moveSpeed_ = speed;
	}

	void resetMousePosition(const SM::Vector2& p) { mousePos_ = p; };

	void setUpVector(const SM::Vector3& up)
	{
		const SM::Matrix view = getViewMatrix();
		SM::Vector3 dir = DirectX::XMVectorNegate(SM::Vector3(view._13, view._23, view._33));

		cameraOrientation_ = DirectX::XMQuaternionRotationMatrix(
			DirectX::XMMatrixLookAtRH(cameraPosition_, DirectX::XMVectorAdd(cameraPosition_, dir), up)
		);
	}

	inline void lookAt(const SM::Vector3& pos, const SM::Vector3& target, const SM::Vector3& up) {
		cameraPosition_ = pos;
		cameraOrientation_ = DirectX::XMQuaternionRotationMatrix(DirectX::XMMatrixLookAtRH(pos, target, up));
	}

public:
	struct Movement
	{
		bool forward_ = false;
		bool backward_ = false;
		bool left_ = false;
		bool right_ = false;
		bool up_ = false;
		bool down_ = false;
		//
		bool fastSpeed_ = false;
	} movement_;

public:
	float mouseSpeed_ = 2.0f;
	float acceleration_ = 114.0f;
	float damping_ = 0.2f;
	float maxSpeed_ = 10.0f;
	float fastCoef_ = 10.0f;

private:
	SM::Vector2 mousePos_ = SM::Vector2{0., 0.};
	SM::Vector3 cameraPosition_ = SM::Vector3(0.0f, 10.0f, 10.0f);
	SM::Quaternion cameraOrientation_ = SM::Quaternion::Identity;
	SM::Vector3 moveSpeed_ = SM::Vector3(0.0f);
	SM::Vector3 up_ = SM::Vector3(0.0f, 0.0f, 1.0f);
};

class CameraPositioner_MoveTo final : public CameraPositionerInterface
{
public:
	CameraPositioner_MoveTo(const SM::Vector3& pos, const SM::Vector3& angles)
		: positionCurrent_(pos)
		, positionDesired_(pos)
		, anglesCurrent_(angles)
		, anglesDesired_(angles)
	{}

	void update(float deltaSeconds, const SM::Vector2& mousePos, bool mousePressed)
	{
		positionCurrent_ += dampingLinear_ * deltaSeconds * (positionDesired_ - positionCurrent_);

		// normalization is required to avoid "spinning" around the object 2pi times
		anglesCurrent_ = clipAngles(anglesCurrent_);
		anglesDesired_ = clipAngles(anglesDesired_);

		// update angles
		anglesCurrent_ -= angleDelta(anglesCurrent_, anglesDesired_) * dampingEulerAngles_ * deltaSeconds;

		// normalize new angles
		anglesCurrent_ = clipAngles(anglesCurrent_);

		DirectX::XMVECTOR a = DirectX::XMVectorMultiply(anglesCurrent_, DirectX::XMVectorReplicate(DirectX::XM_PI / 180.0f));

		DirectX::XMMATRIX rotation = DirectX::XMMatrixRotationRollPitchYaw(a.m128_f32[1], a.m128_f32[0], a.m128_f32[2]);
		DirectX::XMMATRIX translation = DirectX::XMMatrixTranslation(-positionCurrent_.x, -positionCurrent_.y, -positionCurrent_.z);
		currentTransform_ = translation * rotation;
	}

	void setPosition(const SM::Vector3& p) { positionCurrent_ = p; }
	void setAngles(float pitch, float pan, float roll) { anglesCurrent_ = SM::Vector3(pitch, pan, roll); }
	void setAngles(const SM::Vector3& angles) { anglesCurrent_ = angles; }
	void setDesiredPosition(const SM::Vector3& p) { positionDesired_ = p; }
	void setDesiredAngles(float pitch, float pan, float roll) { anglesDesired_ = SM::Vector3(pitch, pan, roll); }
	void setDesiredAngles(const SM::Vector3& angles) { anglesDesired_ = angles; }

	virtual SM::Vector3 getPosition() const override { return positionCurrent_; }
	virtual SM::Matrix getViewMatrix() const override { return currentTransform_; }

public:
	float dampingLinear_ = 10.0f;
	SM::Vector3 dampingEulerAngles_ = SM::Vector3(5.0f, 5.0f, 5.0f);

private:
	SM::Vector3 positionCurrent_ = SM::Vector3(0.0f);
	SM::Vector3 positionDesired_ = SM::Vector3(0.0f);

	/// pitch, pan, roll
	SM::Vector3 anglesCurrent_ = SM::Vector3(0.0f);
	SM::Vector3 anglesDesired_ = SM::Vector3(0.0f);

	SM::Matrix currentTransform_ = SM::Matrix();

	static inline float clipAngle(float d)
	{
		if (d < -180.0f) return d + 360.0f;
		if (d > +180.0f) return d - 360.f;
		return d;
	}

	static inline SM::Vector3 clipAngles(const SM::Vector3& angles)
	{
		return SM::Vector3(
			std::fmod(angles.x, 360.0f),
			std::fmod(angles.y, 360.0f),
			std::fmod(angles.z, 360.0f)
		);
	}

	static inline SM::Vector3 angleDelta(const SM::Vector3& anglesCurrent, const SM::Vector3& anglesDesired)
	{
		const SM::Vector3 d = clipAngles(anglesCurrent) - clipAngles(anglesDesired);
		return SM::Vector3(clipAngle(d.x), clipAngle(d.y), clipAngle(d.z));
	}
};
