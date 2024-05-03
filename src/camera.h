#pragma once
#include <stdbool.h>
#include <cglm/struct/mat4.h>
#include <cglm/struct/vec2.h>
#include <cglm/struct/vec3.h>

extern vec3s camera_position;
extern mat4s camera_view_projection;

void update_camera(float delta);