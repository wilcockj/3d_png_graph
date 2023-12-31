#include <raylib.h>
#include <raymath.h>
#define RLIGHTS_IMPLEMENTATION
#include "rlights.h"
#include <stdio.h>
#include <stdlib.h>

int main(int argc, char *argv[]) {
  // goal is to load image
  // and graph the color of each pixel
  // on 3d graph
  const char *filename = argv[1];
  const int scr_width = 800;
  const int scr_height = 600;
  const float circle_radius = 0.1;
  InitWindow(scr_width, scr_height,
             "raylib [models] example - model animation");
  Camera camera = {0};
  camera.position = (Vector3){10.0f, 10.0f, 10.0f}; // Camera position
  camera.target = (Vector3){0.0f, 0.0f, 0.0f};      // Camera looking at point
  camera.up =
      (Vector3){0.0f, 1.0f, 0.0f}; // Camera up vector (rotation towards target)
  camera.fovy = 45.0f;             // Camera field-of-view Y
  camera.projection = CAMERA_PERSPECTIVE; // Camera mode type

  Vector3 center = {0, 0, 0};
  SetTargetFPS(60); // Set our game to run at 60 frames-per-second
  Image target_image = LoadImage(filename);
  Color *target_colors = LoadImageColors(target_image);
  // maybe instead need to find the unique pixels?
  // to reduce number needed
  int num_pixels = target_image.width * target_image.height;
  printf("There are %d pixels\n", num_pixels);

  Mesh my_pyr = GenMeshPoly(4, 1);
  Mesh my_cube = GenMeshCube(1.0f, 1.0f, 1.0f);
  Mesh my_small_cube = GenMeshCube(0.1f, 0.1f, 0.1f);
  Material matIntances = LoadMaterialDefault();
  Matrix *transforms = (Matrix *)RL_CALLOC(num_pixels, sizeof(Matrix));
  Material matDefault = LoadMaterialDefault();

  // Load lighting shader
  Shader shader = LoadShader("lighting_instancing.vs", "lighting.fs");

  // Get shader locations
  shader.locs[SHADER_LOC_MATRIX_MVP] = GetShaderLocation(shader, "mvp");
  shader.locs[SHADER_LOC_VECTOR_VIEW] = GetShaderLocation(shader, "viewPos");
  shader.locs[SHADER_LOC_MATRIX_MODEL] =
      GetShaderLocationAttrib(shader, "instanceTransform");

  // Set shader value: ambient light level
  int ambientLoc = GetShaderLocation(shader, "ambient");
  SetShaderValue(shader, ambientLoc, (float[4]){0.2f, 0.2f, 0.2f, 1.0f},
                 SHADER_UNIFORM_VEC4);

  // Create one light
  CreateLight(LIGHT_DIRECTIONAL, (Vector3){50.0f, 50.0f, 0.0f}, Vector3Zero(),
              WHITE, shader);
  matDefault.maps[MATERIAL_MAP_DIFFUSE].color = BLUE;

  Material matInstances = LoadMaterialDefault();
  matInstances.shader = shader;
  matInstances.maps[MATERIAL_MAP_DIFFUSE].color = RED;

  for (int i = 0; i < num_pixels; i++) {
    Color cur_color = target_colors[i];
    Vector3 pos;
    pos.x = (double)cur_color.r / 255 * 5;
    pos.y = (double)cur_color.g / 255 * 5;
    pos.z = (double)cur_color.b / 255 * 5;
    //    printf("%f,%f,%f\n", pos.x, pos.y, pos.z);
    Matrix translation = MatrixTranslate(pos.x, pos.y, pos.z);
    Vector3 axis = Vector3Normalize((Vector3){0, 0, 0});
    Matrix rotation = MatrixRotate(axis, 0);
    transforms[i] = MatrixTranslate(pos.x, pos.y, pos.z);
  }

  //--------------------------------------------------------------------------------------

  // Main game loop
  while (!WindowShouldClose()) // Detect window close button or ESC key
  {
    // Update
    //----------------------------------------------------------------------------------
    UpdateCamera(&camera, CAMERA_ORBITAL);

    // Update the light shader with the camera view position
    // float cameraPos[3] = {camera.position.x, camera.position.y,
    //                      camera.position.z};
    // SetShaderValue(shader, shader.locs[SHADER_LOC_VECTOR_VIEW], cameraPos,
    //              SHADER_UNIFORM_VEC3);

    // Draw
    //----------------------------------------------------------------------------------
    BeginDrawing();

    ClearBackground(RAYWHITE);

    BeginMode3D(camera);

    DrawGrid(10, 1.0f); // Draw a grid
    DrawMeshInstanced(my_small_cube, matInstances, transforms, num_pixels);

    EndMode3D();

    DrawFPS(10, 10);

    EndDrawing();
    //----------------------------------------------------------------------------------
  }
  UnloadImageColors(target_colors);
  return EXIT_SUCCESS;
}
