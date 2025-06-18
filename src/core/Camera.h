#pragma once

#include "common.h"
#include <StandardTypes.h>

namespace Cravillac
{
	class Camera
	{
	public:
		Camera();
		~Camera() = default;

		void InitAsPerspective(const f32 angle, const f32 width, const f32 height, const f32 nearPlane = 0.1f, const f32 farPlane = 1000.0f);
		void InitAsOrthographic(const f32 width, const f32 height, const f32 nearPlane = 0.1f, const f32 farPlane = 1000.0f);
		void OnResize(u32 width, u32 height);

		void Translate(SM::Vector3 direction);
		void Rotate(SM::Vector3 axis, f32 angle);
		void LookAt(SM::Vector3 target);

		const SM::Vector3 GetLookAtTarget() const;

		inline const SM::Vector3& GetPosition() const noexcept { return m_position; }
		inline const SM::Vector3& GetTarget() const noexcept { return m_target; }
		inline const SM::Vector3& GetUp() const noexcept { return m_up; }
		inline const f32& GetAngle() const noexcept { return m_angle; }
		inline const f32& GetNearPlane() const noexcept { return m_nearPlane; }
		inline const f32& GetFarPlane() const noexcept { return m_farPlane; }
		inline const SM::Matrix& GetViewMatrix() const { return m_view; }
		inline const SM::Matrix& GetProjectionMatrix() const { return m_proj; }
		inline const SM::Matrix& GetOrthoMatrix() const { return m_ortho; }

		float GetYaw() const { return m_yaw; }
		float GetPitch() const { return m_pitch; }
		void SetYaw(float yaw) { m_yaw = yaw; }
		void SetPitch(float pitch) { m_pitch = pitch; }

		void SetPosition(const SM::Vector3& position);
		void SetAngle(const f32 angle);
		void SetNearPlane(const f32 nearPlane);
		void SetFarPlane(const f32 farPlane);

		void InitViewMatrix();

		SM::Vector3 m_position;
		SM::Vector3 m_target;
		SM::Vector3 m_up;
		f32         m_angle;
		f32         m_width;
		f32         m_height;
		f32         m_nearPlane;
		f32         m_farPlane;
		SM::Matrix  m_view;
		SM::Matrix  m_proj;
		SM::Matrix  m_ortho;
		float m_yaw = -90.0f;
		float m_pitch = 0.0f;
	};
}