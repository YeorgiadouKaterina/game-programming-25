#ifndef ITU_LIB_DEBUG_UI_HPP
#define ITU_LIB_DEBUG_UI_HPP

#define ENABLE_BROKEN_TRANSFORM_INSPECTOR

void itu_debug_ui_render_transform(SDLContext* context, void* data);
void itu_debug_ui_render_sprite(SDLContext* context, void* data);
void itu_debug_ui_render_physicsdata(SDLContext* context, void* data);
void itu_debug_ui_render_physicsstaticdata(SDLContext* context, void* data);
void itu_debug_ui_render_shapedata(SDLContext* context, void* data);

#endif // ITU_LIB_DEBUG_UI_HPP

// TMP
#define ITU_LIB_DEBUG_UI_IMPLEMENTATION

#if (defined ITU_LIB_DEBUG_UI_IMPLEMENTATION) || (defined ITU_UNITY_BUILD)

#ifndef ITU_UNITY_BUILD
#include <imgui/imgui.h>
#include <box2d/box2d.h>

#endif

void itu_debug_ui_render_transform(SDLContext* context, void* data)
{
	Transform* data_transform  = (Transform*)data;
	ImGui::DragFloat2("position", &data_transform->position.x);
	ImGui::DragFloat2("scale", &data_transform->scale.x);

	float rotation_deg = data_transform->rotation * RAD_2_DEG;
	if(ImGui::DragFloat("rotation", &rotation_deg))
		data_transform->rotation = rotation_deg * DEG_2_RAD;

	itu_lib_render_draw_world_point(context, data_transform->position, 5, COLOR_YELLOW);
}

void itu_debug_ui_render_sprite(SDLContext* context, void* data)
{
	Sprite* data_sprite = (Sprite*)data;

	itu_sys_rstorage_debug_render_texture(data_sprite->texture, &data_sprite->texture, &data_sprite->rect);

	ImGui::DragFloat4("texture rect", &data_sprite->rect.x);
	ImGui::DragFloat2("pivot", &data_sprite->pivot.x);

	ImGui::ColorEdit4("tint", &data_sprite->tint.r);
	ImGui::Checkbox("Flip Hor.", &data_sprite->flip_horizontal);
}

void itu_debug_ui_render_physicsdata(SDLContext* context, void* data)
{
	PhysicsData* data_body = (PhysicsData*)data;

	ImGui::DragFloat2("velocity", &data_body->velocity.x);
	ImGui::DragFloat("torque", &data_body->torque);

	// TODO show definition data (either here, or in a more appropriate place)
}

void itu_debug_ui_render_physicsstaticdata(SDLContext* context, void* data)
{
	// nothing to show here at runtime

	// TODO show definition data (either here, or in a more appropriate place)

}

const char* const b2_shape_names[6] = 
{
	"circle",
	"capsule",
	"segment (not supported)",
	"polygon",
	"chainSegment (not supported)",
	"invalid shape"
};

// NOTE: this function renders shapes using current b2d transform information, so it looks like the
//       shapes "stutter" a bit, since we are doing a smooth interpolation between b2d states for rendering
void itu_debug_ui_render_shapedata(SDLContext* context, void* data)
{
	ShapeData* data_shape = (ShapeData*)data;
	
	b2ShapeId id_shape = data_shape->shape_id;
	b2BodyId id_body = b2Shape_GetBody(id_shape);
	b2Transform transform = b2Body_GetTransform(id_body);

	b2ShapeType shape_type = b2Shape_GetType(id_shape);
	int shape_idx = (int)shape_type;
	int tmp = (sizeof(b2_shape_names) / sizeof((b2_shape_names)[0]));
	ImGui::Combo("type", &shape_idx, b2_shape_names, tmp);

	bool dirty_shape = shape_idx != shape_type;
	bool dirty_data = false;

	// show debug ui based on CURRENT shape
	switch(shape_type)
	{
		case b2_circleShape:
		{
			b2Circle circle = b2Shape_GetCircle(id_shape);
			transform.p = b2TransformPoint(transform, circle.center);
			fn_box2d_wrapper_draw_circle(transform , circle.radius, b2_colorLightGreen, context);

			dirty_data |= ImGui::DragFloat2("center", &circle.center.x);
			dirty_data |= ImGui::DragFloat("radius", &circle.radius);
			if(dirty_data)
				b2Shape_SetCircle(id_shape, &circle);
			break;
		}
		case b2_polygonShape:
		{
			b2Polygon poly = b2Shape_GetPolygon(id_shape);
			fn_box2d_wrapper_draw_polygon(transform, poly.vertices, poly.count, poly.radius, b2_colorLightGreen, context);

			dirty_data |= ImGui::DragInt("vertex count", &poly.count, 1, 3, B2_MAX_POLYGON_VERTICES);
			for(int i = 0; i < poly.count; ++i)
			{
				char buf[4];
				SDL_snprintf(buf, 4, "v%d", i);
				dirty_data |= ImGui::DragFloat2(buf, &poly.vertices[i].x);
			}
			if(dirty_data)
				b2Shape_SetPolygon(id_shape, &poly);
			break;
		}
		case b2_capsuleShape:
		{
			b2Capsule capsule = b2Shape_GetCapsule(id_shape);
			b2Vec2 p1 = b2TransformPoint(transform, capsule.center1);
			b2Vec2 p2 = b2TransformPoint(transform, capsule.center2);
			fn_box2d_wrapper_draw_capsule(p1, p2, capsule.radius, b2_colorLightGreen, context);

			dirty_data |= ImGui::DragFloat2("p1", &capsule.center1.x);
			dirty_data |= ImGui::DragFloat2("p2", &capsule.center1.x);
			dirty_data |= ImGui::DragFloat("radius", &capsule.radius);

			if(dirty_data)
				b2Shape_SetCapsule(id_shape, &capsule);
			break;
		}
		default:
		{
			ImGui::Text("shape not supported");
			break;
		}
	}

	if(dirty_shape)
		// update data based on NEWLY SELECTED shape
		switch(shape_idx)
		{
			case b2_circleShape:
			{
				b2Circle circle = { 0 };
				circle.radius = 0.5f;
				b2Shape_SetCircle(id_shape, &circle);
				break;
			}
			case b2_polygonShape:
			{
				b2Polygon poly = b2MakeBox(0.5f, 0.5f);
				b2Shape_SetPolygon(id_shape, &poly);
				break;
			}
			case b2_capsuleShape:
			{
				b2Capsule capsule = { 0 };
				capsule.radius = 0.5f;
				b2Shape_SetCapsule(id_shape, &capsule);
				break;
			}
		}
}

bool itu_debug_ui_render_mat4_decomposed(const char* label, glm::mat4* matrix, bool readonly)
{
	bool ret = false;
	
	ImGui::Text(label);
	ImGui::PushID(label);
	glm::vec3 pos, rot, scale;
	{
		itu_lib_math_decompose_transform(*matrix, &pos, &rot, &scale);
		rot = rot * RAD_2_DEG;
	}

	
	bool dirty = false;
// TODO fix matrix decomposition (rotational error for some weird combination of angles)
// you can define ENABLE_BROKEN_TRANSFORM_INSPECTOR for testing purposes, but know that it will break sometimes
#ifndef ENABLE_BROKEN_TRANSFORM_INSPECTOR
	readonly = true;
#endif
	if(readonly)
	{
		ImGui::InputFloat3("tanslation (readonly)", &pos.x, "%.3f", ImGuiInputTextFlags_ReadOnly);
		ImGui::InputFloat3("rotation (readonly)", &rot.x, "%.3f"  , ImGuiInputTextFlags_ReadOnly);
		ImGui::InputFloat3("scale (readonly)", &scale.x, "%.3f"   , ImGuiInputTextFlags_ReadOnly);
	}
	else
	{
		dirty |= ImGui::DragFloat3("tanslation (readonly)", &pos.x  );
		dirty |= ImGui::DragFloat3("rotation (readonly)"  , &rot.x  );
		dirty |= ImGui::DragFloat3("scale (readonly)"     , &scale.x);

		if(dirty)
		{
			glm::vec3 rot_rad = rot * DEG_2_RAD;
			itu_lib_math_assemble_transform(matrix, pos, rot_rad, scale);
		}
	}
	
	
	ImGui::PopID();
	return dirty;
}

void itu_debug_ui_render_transform3D(SDLContext* context, void* data)
{
	Transform3D* data_transform = (Transform3D*)data;

	static bool show_global = false;
	ImGui::Checkbox("show global (readonly)", &show_global);
	ImGui::SameLine();
	if(ImGui::Button("Reset rotation"))
		itu_lib_math_reset_rotation(&data_transform->local);

	if(show_global)
		itu_debug_ui_render_mat4_decomposed("transform global (readonly)", &data_transform->global, true);
	else
		itu_debug_ui_render_mat4_decomposed("transform local", &data_transform->local, false);
}

void itu_debug_ui_render_meshcomponent(SDLContext* context, void* data)
{
	MeshComponent* data_mesh = (MeshComponent*)data;

	Model3D* model3d_data = itu_sys_rstorage_model3d_get_ptr(data_mesh->id_model3d);
	if(itu_sys_rstorage_debug_render_model3d(model3d_data, &model3d_data))
	{
		data_mesh->id_model3d = itu_sys_rstorage_model3d_from_ptr(model3d_data);
	}
}

void itu_debug_ui_render_cameracomponent(SDLContext* context, void* data)
{
	CameraComponent* camera_data = (CameraComponent*)data;

	bool dirty = false;

	ImGui::Combo("type", (int*)&camera_data->type, camera_type_names, CAMERA_TYPE_COUNT, -1);
	if(camera_data->type == CAMERA_TYPE_PERSPECTIVE)
	{
		float fow_deg = camera_data->fow * RAD_2_DEG;
		if(ImGui::DragFloat("field of view angle", &fow_deg, 1, 0, 90))
			camera_data->fow = fow_deg * DEG_2_RAD;
	}
	else if(camera_data->type == CAMERA_TYPE_ORTHOGRAPHIC)
	{
		ImGui::DragFloat("zoom", &camera_data->zoom);
	}
	ImGui::DragFloat("near", &camera_data->near, 1, 0.01f, camera_data->far);
	ImGui::DragFloat("far", &camera_data->far, 1, camera_data->near, 100.0f);
}

#endif // (defined ITU_LIB_DEBUG_UI_IMPLEMENTATION) || (defined ITU_UNITY_BUILD)