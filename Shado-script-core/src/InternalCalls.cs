﻿using System;
using System.Runtime.CompilerServices;

namespace Shado
{
	public static class InternalCalls
	{
		[MethodImplAttribute(MethodImplOptions.InternalCall)]
		internal extern static bool Entity_HasComponent(ulong entityID, Type componentType);
		[MethodImpl(MethodImplOptions.InternalCall)]
		internal extern static void Entity_RemoveComponent(ulong entityID, Type componentType);

        [MethodImpl(MethodImplOptions.InternalCall)]
		internal extern static void Entity_AddComponent(ulong entityID, Type componentType);

		[MethodImplAttribute(MethodImplOptions.InternalCall)]
		internal extern static ulong Entity_FindEntityByName(string name);
		[MethodImplAttribute(MethodImplOptions.InternalCall)]
		internal extern static object GetScriptInstance(ulong entityID);

		/**
		 * Tag
		 **/
		[MethodImpl(MethodImplOptions.InternalCall)]
		internal extern static void TagComponent_GetTag(ulong entityID, out string tag);

        [MethodImpl(MethodImplOptions.InternalCall)]
		internal extern static void TagComponent_SetTag(ulong entityID, ref string tag);

		/**
		 * Transform
		 **/
		[MethodImpl(MethodImplOptions.InternalCall)]
		internal extern static void TransformComponent_GetTranslation(ulong entityID, out Vector3 translation);

		[MethodImpl(MethodImplOptions.InternalCall)]
		internal extern static void TransformComponent_SetTranslation(ulong entityID, ref Vector3 translation);

		[MethodImpl(MethodImplOptions.InternalCall)]
		internal extern static void TransformComponent_GetRotation(ulong entityID, out Vector3 rotation);

		[MethodImpl(MethodImplOptions.InternalCall)]
		internal extern static void TransformComponent_SetRotation(ulong entityID, ref Vector3 rotation);

		[MethodImpl(MethodImplOptions.InternalCall)]
		internal extern static void TransformComponent_GetScale(ulong entityID, out Vector3 scale);

		[MethodImpl(MethodImplOptions.InternalCall)]
		internal extern static void TransformComponent_SetScale(ulong entityID, ref Vector3 scale);

		/**
		 * SpriteRenderer
		 */
		[MethodImpl(MethodImplOptions.InternalCall)]
		internal extern static void SpriteRendererComponent_GetColour(ulong entityID, out Vector4 result, string klass);
		[MethodImpl(MethodImplOptions.InternalCall)]
		internal extern static void SpriteRendererComponent_SetColour(ulong entityID, ref Vector4 result, string klass);

		[MethodImpl(MethodImplOptions.InternalCall)]
		internal extern static Texture2D SpriteRendererComponent_GetTexture(ulong entityID, string klass);
		[MethodImpl(MethodImplOptions.InternalCall)]
		internal extern static void SpriteRendererComponent_SetTexture(ulong entityID, IntPtr native, string klass);

		[MethodImpl(MethodImplOptions.InternalCall)]
		internal extern static void SpriteRendererComponent_GetTilingFactor(ulong entityID, out float result, string klass);
		[MethodImpl(MethodImplOptions.InternalCall)]
		internal extern static void SpriteRendererComponent_SetTilingFactor(ulong entityID, ref float result, string klass);

		/**
		 * CircleRenderer
		 */
		[MethodImpl(MethodImplOptions.InternalCall)]
		internal extern static void CircleRendererComponent_GetFloatValue(ulong entityId, string fieldName, out float val);

		[MethodImpl(MethodImplOptions.InternalCall)]
		internal extern static void CircleRendererComponent_SetFloatValue(ulong entityId, string fieldName, ref float val);
		
		/**
		 * Rigidbody2D
		 **/
		[MethodImplAttribute(MethodImplOptions.InternalCall)]
		internal extern static void Rigidbody2DComponent_ApplyLinearImpulse(ulong entityID, ref Vector2 impulse, ref Vector2 point, bool wake);
		[MethodImplAttribute(MethodImplOptions.InternalCall)]
		internal extern static void Rigidbody2DComponent_GetLinearVelocity(ulong entityID, out Vector2 linearVelocity);
		[MethodImplAttribute(MethodImplOptions.InternalCall)]
		internal extern static RigidBody2DComponent.BodyType Rigidbody2DComponent_GetType(ulong entityID);
		[MethodImplAttribute(MethodImplOptions.InternalCall)]
		internal extern static void Rigidbody2DComponent_SetType(ulong entityID, RigidBody2DComponent.BodyType type);
		[MethodImplAttribute(MethodImplOptions.InternalCall)]
		internal extern static void Rigidbody2DComponent_ApplyLinearImpulseToCenter(ulong entityID, ref Vector2 impulse, bool wake);

		/**
		 * BoxCollider2D
		 */
		[MethodImpl(MethodImplOptions.InternalCall)]
		internal extern static void BoxCollider2DComponent_GetVec2(ulong entityId, BoxCollider2DComponent.FieldToQuery field, out Vector2 offset, BoxCollider2DComponent.FieldToQuery componentType);
		[MethodImpl(MethodImplOptions.InternalCall)]
		internal extern static void BoxCollider2DComponent_SetVec2(ulong entityId, BoxCollider2DComponent.FieldToQuery field, ref Vector2 offset, BoxCollider2DComponent.FieldToQuery componentType);
		[MethodImpl(MethodImplOptions.InternalCall)]
		internal extern static void BoxCollider2DComponent_GetFloat(ulong entityId, BoxCollider2DComponent.FieldToQuery field, out float offset, BoxCollider2DComponent.FieldToQuery componentType);
		[MethodImpl(MethodImplOptions.InternalCall)]
		internal extern static void BoxCollider2DComponent_SetFloat(ulong entityId, BoxCollider2DComponent.FieldToQuery field, ref float offset, BoxCollider2DComponent.FieldToQuery componentType);

		/**
		 *  CameraComponent
		 */
		[MethodImpl(MethodImplOptions.InternalCall)]
		internal extern static void CameraComponent_GetPrimary(ulong entityId, out bool primary);
		[MethodImpl(MethodImplOptions.InternalCall)]
		internal extern static void CameraComponent_SetPrimary(ulong entityId, ref bool primary);
		[MethodImpl(MethodImplOptions.InternalCall)]
		internal extern static void CameraComponent_GetType(ulong entityId, out CameraComponent.Type type);
		[MethodImpl(MethodImplOptions.InternalCall)]
		internal extern static void CameraComponent_SetType(ulong entityId, ref CameraComponent.Type type);
		[MethodImpl(MethodImplOptions.InternalCall)]
		internal extern static void CameraComponent_SetViewport(ulong entityID, uint width, uint height);
		/**
		 * Input
		 **/
		[MethodImplAttribute(MethodImplOptions.InternalCall)]
		internal extern static bool Input_IsKeyDown(KeyCode keycode);

		/**
		 * Texture2D
		 */
		[MethodImpl(MethodImplOptions.InternalCall)]
		internal extern static Texture2D Texture2D_Create(string filepath);

		[MethodImpl(MethodImplOptions.InternalCall)]
		internal extern static void Texture2D_Destroy(IntPtr native);

		[MethodImpl(MethodImplOptions.InternalCall)]
		internal extern static void Texture2D_Reset(IntPtr ptr, string filepath, out IntPtr newHandle);

		/**
		 * Shader
		 */
		[MethodImpl(MethodImplOptions.InternalCall)]
		internal extern static void Shader_CreateShader(string filepath, out IntPtr native);

		[MethodImpl(MethodImplOptions.InternalCall)]
		internal extern static void Shader_Destroy(IntPtr native);

		[MethodImpl(MethodImplOptions.InternalCall)]
		internal extern static void Shader_SetInt(IntPtr natiive, string name, ref int value);
		[MethodImpl(MethodImplOptions.InternalCall)]
		internal extern static void Shader_SetIntArray(IntPtr natiive, string name, int[] value);
		[MethodImpl(MethodImplOptions.InternalCall)]
		internal extern static void Shader_SetFloat(IntPtr natiive, string name, ref float value);
		[MethodImpl(MethodImplOptions.InternalCall)]
		internal extern static void Shader_SetFloat3(IntPtr natiive, string name, ref Vector3 value);
		[MethodImpl(MethodImplOptions.InternalCall)]
		internal extern static void Shader_SetFloat4(IntPtr natiive, string name, ref Vector4 value);
		[MethodImpl(MethodImplOptions.InternalCall)]
		internal extern static void Shader_Reset(IntPtr native, string filepath, out IntPtr value);

		/**
		 * Log
		 */
		[MethodImpl(MethodImplOptions.InternalCall)]
		internal extern static void Log_Log(string message, Log.Type type);

		/**
		 * Renderer (for debugging)
		 */
		[MethodImpl(MethodImplOptions.InternalCall)]
		internal extern static void Renderer_DrawQuad(ref Vector3 position, ref Vector3 scale, ref Vector4 colour);
		[MethodImpl(MethodImplOptions.InternalCall)]
		internal extern static void Renderer_DrawQuadShader(ref Vector3 position, ref Vector3 scale, ref Vector4 colour, IntPtr shader);
		[MethodImpl(MethodImplOptions.InternalCall)]
		internal extern static void Renderer_DrawRotatedQuad(ref Vector3 position, ref Vector3 scale, ref Vector3 rotation, ref Vector4 colour);
		[MethodImpl(MethodImplOptions.InternalCall)]
		internal extern static void Renderer_DrawLine(ref Vector3 p0, ref Vector3 p1, ref Vector4 colour);
	}
}