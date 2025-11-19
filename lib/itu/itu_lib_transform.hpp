#ifndef ITU_LIB_TRANSFORM_HPP
#define ITU_LIB_TRANSFORM_HPP

#define TRANSFORM_HIERARCHY_MAX_DEPTH 16

#ifndef ITU_UNITY_BUILD
#include <itu_entity_storage.hpp>
#include <glm/glm.hpp>
#endif

struct Transform3D
{
	int TMP_id;
	Transform3D* parent;
	Transform3D* child_first;
	Transform3D* child_last;
	Transform3D* sibling_prev;
	Transform3D* sibling_next;

	glm::mat4 local;
	glm::mat4 global;
};

Transform3D* transform3D_add(ITU_EntityId id, ITU_EntityId parent);
void transform3D_reparent(ITU_EntityId id, ITU_EntityId new_parent);

int compare_transform_breadth_first(const void *a, const void* b);
int compare_transform_depth_first(const void *a, const void* b);

void itu_lib_transform_update_globals();

#endif // ITU_LIB_TRANSFORM_HPP

// TMP
#define ITU_LIB_TRANSFORM_IMPLEMENTATION
#if (defined ITU_LIB_TRANSFORM_IMPLEMENTATION) || (defined ITU_UNITY_BUILD)

static Transform3D TMP_transform_root = 
{
	-1
};

void transform_hierarchy_remove(Transform3D* ptr, Transform3D* parent_old);
void transform_hierarchy_add(Transform3D* ptr, Transform3D* parent_old);
bool transform_is_offspring_of(Transform3D* ptr, Transform3D* ptr_other);
void transform_hierarchy_refresh_before_deletion(Transform3D* ptr_to_be_deleted);

void transform_hierarchy_remove(Transform3D* ptr)
{
	if(!ptr->parent)
		return;
	Transform3D* parent = ptr->parent;

	if(parent->child_first == ptr)
	{
		parent->child_first = ptr->sibling_next;
		if(parent->child_first)
			parent->child_first->sibling_prev = NULL;
	}
	else if(parent->child_last == ptr)
	{
		parent->child_last = ptr->sibling_prev;
		if(parent->child_last)
			parent->child_last->sibling_next = NULL;
	}
	else
	{
		if(ptr->sibling_prev)
			ptr->sibling_prev->sibling_next = ptr->sibling_next;
		if(ptr->sibling_next)
			ptr->sibling_next->sibling_prev = ptr->sibling_prev;
	}

	// clean up siblings
	ptr->sibling_next = ptr->sibling_prev = NULL;
}

void transform_hierarchy_add(Transform3D* ptr, Transform3D* parent_new)
{
	if(!parent_new)
		parent_new = &TMP_transform_root;

	if(!parent_new->child_first)
	{
		// parent has no children
		ptr->parent = parent_new;
		ptr->sibling_next = ptr->sibling_prev = NULL;
		parent_new->child_first = parent_new->child_last = ptr;
	}
	else
	{
		// parent with siblings, append to the end
		ptr->parent = parent_new;
		ptr->sibling_next = NULL;
		ptr->sibling_prev = parent_new->child_last;
		parent_new->child_last->sibling_next = ptr;
		parent_new->child_last = ptr;
	}
}

Transform3D* transform3D_add(ITU_EntityId id, ITU_EntityId parent)
{
	static int tmp_id = 0;
	Transform3D empty = { 0 };
	empty.TMP_id = tmp_id++;
	empty.local = glm::mat4(1.0f);
	entity_add_component(id, Transform3D, empty);

	Transform3D* ptr = entity_get_data(id, Transform3D);
	Transform3D* ptr_parent = entity_get_data(parent, Transform3D);

	if(itu_entity_equals(id, parent))
		// cannot add a transform to itself! fallback to no parent
		parent = ITU_ENTITY_ID_NULL;

	transform_hierarchy_add((Transform3D*)ptr, (Transform3D*)ptr_parent);

	return ptr;
}

void transform3D_reparent(ITU_EntityId id, ITU_EntityId new_parent)
{
	
	Transform3D* ptr = entity_get_data(id, Transform3D);
	Transform3D* ptr_parent = entity_get_data(new_parent, Transform3D);

	if(transform_is_offspring_of(ptr_parent, ptr))
	{
		SDL_Log("WARNING cannot reparent node to one of its offspring!");
		return;
	}

	transform_hierarchy_remove(ptr);
	transform_hierarchy_add(ptr, ptr_parent);
}

bool transform_is_offspring_of(Transform3D* ptr, Transform3D* ptr_other)
{
	if(ptr == ptr_other)
		return true;
	if(ptr == NULL || ptr_other == NULL)
		return false;

	if(ptr->child_first && transform_is_offspring_of(ptr->child_first, ptr_other))
		return true;

	if(ptr->sibling_next && transform_is_offspring_of(ptr->sibling_next, ptr_other))
		return true;

	return false;
}

void transform_hierarchy_refresh_before_deletion(Transform3D* ptr_old, Transform3D* ptr_new)
{
	if(ptr_old == ptr_new)
	{
		return;
	}

	if(ptr_old->sibling_prev)
		ptr_old->sibling_prev->sibling_next = ptr_new;
	if(ptr_old->sibling_next)
		ptr_old->sibling_next->sibling_prev = ptr_new;

	Transform3D* child = ptr_old->child_first;
	while(child)
	{
		child->parent = ptr_new;
		child = child->sibling_next;
	}

	if(ptr_new->sibling_prev)
		ptr_new->sibling_prev->sibling_next = ptr_new->sibling_next;
	if(ptr_new->sibling_next)
		ptr_new->sibling_next->sibling_prev = ptr_new->sibling_prev;

	child = ptr_new->child_first;
	while(child)
	{
		child->parent = ptr_old;
		child = child->sibling_next;
	}
}

void itu_lib_transform_update_globals()
{
	Transform3D* stack[TRANSFORM_HIERARCHY_MAX_DEPTH] = { 0 };
	stack[0] = (Transform3D*)TMP_transform_root.child_first;
	int stack_depth = 0;

	while(stack_depth >= 0)
	{
		Transform3D* curr = stack[stack_depth];
		if(!curr)
		{
			--stack_depth;
			if(stack_depth >= 0)
				stack[stack_depth] = stack[stack_depth]->sibling_next;
			continue;
		}

		if(stack_depth > 0)
			curr->global = curr->parent->global * curr->local;
		else
			curr->global = curr->local;
		
		if(curr->child_first && stack_depth < TRANSFORM_HIERARCHY_MAX_DEPTH)
			stack[++stack_depth] = curr->child_first;
		else
			stack[stack_depth] = curr->sibling_next;
	}
}

#endif // (defined ITU_LIB_TRANSFORM_IMPLEMENTATION) || (defined ITU_UNITY_BUILD)