#include "colorutil.h"
#include <raylib.h>
#include <raymath.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MAX_SAMPLES 100000
#define MAX_COLORS 50000
#define CUBE_SIDE_LEN 0.05f

#define SCREEN_WIDTH 800
#define SCREEN_HEIGHT 600

void Draw_Image_In_Region(Texture2D tex, Rectangle region) {
  Rectangle src = (Rectangle){0, 0, tex.width, tex.height};

  bool height_greater = tex.width < tex.height;
  Rectangle dest;
  float image_ratio = (float)tex.height / (float)tex.width;

  // TODO: this is messed for vertical images
  // also add define for the region to display images
  // in
  if (height_greater) {
    size_t new_width = region.width / image_ratio;
    dest = (Rectangle){(region.x + region.width - new_width), region.y,
                       new_width, region.height};
  } else {
    dest = (Rectangle){region.x, 0, region.width, region.height * image_ratio};
  }

  DrawTexturePro(tex, src, dest, (Vector2){0, 0}, 0, WHITE);
}

bool color_in_list(Color cur_color, uint8_t *drawn_pixel_map) {
  // use cur color to see what to check
  unsigned int index = (cur_color.r / 255.0f) + (cur_color.g / 255.0f) * (256) +
                       (cur_color.b / 255.0f) * (256 * 256);
  unsigned int byte_index = index / (8);
  unsigned int cur_bitfield = drawn_pixel_map[byte_index];
  bool present = (cur_bitfield >> (index - (byte_index * 8))) & 0x1;
  if (!present) {
    // need to set in pixel map
    drawn_pixel_map[byte_index] |= 1 << (index - (byte_index * 8));
  }
  return present;
}

int populate_color_list(Image target_image, Color *color_list,
                        uint8_t *drawn_pixel_map) {
  int color_cnt = 0;
  for (int i = 0; i < MAX_SAMPLES; i++) {
    if (color_cnt > MAX_COLORS) {
      break;
    }
    Vector2 coord = {rand() % target_image.width, rand() % target_image.height};
    Color color = GetImageColor(target_image, coord.x, coord.y);
    if (!color_in_list(color, drawn_pixel_map)) {
      color_list[color_cnt++] = color;
    }
  }
  return color_cnt;
}

void load_transforms_from_color_list(Matrix *transform_list, Color *color_list,
                                     int color_cnt, unsigned int quadrant) {
  Vector3 quadrant_lookup[4] = {(Vector3){5, 5, 5}, (Vector3){-5, 5, -5},
                                (Vector3){5, 5, -5}, (Vector3){-5, 5, 5}};
  for (int i = 0; i < color_cnt; i++) {
    Color cur_color = color_list[i];
    Vector3 pos;
    pos.x = ((double)cur_color.r / 255) * quadrant_lookup[quadrant].x;
    pos.y = ((double)cur_color.g / 255) * quadrant_lookup[quadrant].y;
    pos.z = ((double)cur_color.b / 255) * quadrant_lookup[quadrant].z;

    transform_list[i] = MatrixTranslate(pos.x, pos.y, pos.z);
  }
}

struct image_info process_image(Image target_image) {

  struct image_info info;
  info.drawn_pixel_map = malloc((256 * 256 * 256) / (8 * sizeof(uint8_t)));
  info.num_pixels = target_image.width * target_image.height;
  info.color_list = malloc(info.num_pixels * sizeof(Color));

  info.color_cnt =
      populate_color_list(target_image, info.color_list, info.drawn_pixel_map);

  printf("found %d unique colors\n", info.color_cnt);

  info.transform_list = malloc(4 * sizeof(Matrix));
  for (int i = 0; i < 4; i++) {
    info.transform_list[i] = malloc(sizeof(Matrix) * info.color_cnt);
  }

  for (int i = 0; i < 4; i++) {
    load_transforms_from_color_list(info.transform_list[i], info.color_list,
                                    info.color_cnt, i);
  }

  return info;
}

int main(int argc, char *argv[]) {
  // goal is to load image
  // and graph the color of each pixel
  // on 3d graph
  const int scr_width = SCREEN_WIDTH;
  const int scr_height = SCREEN_HEIGHT;
  const float circle_radius = 0.1;
  const float graph_limit = 5;
  srand(1);
  Image target_image;

  if (argc > 1) {
    const char *filename = argv[1];
    if (!FileExists(filename)) {
      printf("file %s does not exist\n", filename);
      exit(1);
    }
    target_image = LoadImage(filename);
  } else {
    // if no file is provided use default image
    target_image = LoadImage("resources/oceansmall.png");
  }

  InitWindow(scr_width, scr_height, "image color grapher");
  SetTargetFPS(120);

  Texture2D target_image_tex = LoadTextureFromImage(target_image);

  Camera camera = {0};
  camera.position = (Vector3){10.0f, 10.0f, 10.0f}; // Camera position
  camera.target = (Vector3){0.0f, 0.0f, 0.0f};      // Camera looking at point
  camera.up =
      (Vector3){0.0f, 1.0f, 0.0f}; // Camera up vector (rotation towards target)
  camera.fovy = 45.0f;             // Camera field-of-view Y
  camera.projection = CAMERA_PERSPECTIVE; // Camera mode type

  Vector3 center = {0, 0, 0};

  // maybe instead need to find the unique pixels?
  // to reduce number needed
  //

  Mesh my_pyr = GenMeshPoly(4, 1);
  Mesh my_cube = GenMeshCube(1.0f, 1.0f, 1.0f);
  Mesh my_small_sphere = GenMeshSphere(
      CUBE_SIDE_LEN, 4,
      8); // GenMeshCube(CUBE_SIDE_LEN, CUBE_SIDE_LEN, CUBE_SIDE_LEN);
  Material matIntances = LoadMaterialDefault();

  Material matDefault = LoadMaterialDefault();

  // Load lighting shader
  Shader shader = LoadShader("resources/lighting_instancing_100.vs",
                             "resources/lighting_100.fs");

  // Get shader locations
  shader.locs[SHADER_LOC_MATRIX_MVP] = GetShaderLocation(shader, "mvp");
  shader.locs[SHADER_LOC_VECTOR_VIEW] = GetShaderLocation(shader, "viewPos");
  shader.locs[SHADER_LOC_MATRIX_MODEL] =
      GetShaderLocationAttrib(shader, "instanceTransform");

  // Set shader value: ambient light level
  int ambientLoc = GetShaderLocation(shader, "ambient");
  SetShaderValue(shader, ambientLoc, (float[4]){0.2f, 0.2f, 0.2f, 1.0f},
                 SHADER_UNIFORM_VEC4);

  Material matInstances = LoadMaterialDefault();
  matInstances.shader = shader;
  printf("making color list\n");

  struct image_info info = process_image(target_image);

  //--------------------------------------------------------------------------------------

  // Main game loop
  while (!WindowShouldClose()) // Detect window close button or ESC key
  {
    // Update
    //----------------------------------------------------------------------------------
    UpdateCamera(&camera, CAMERA_ORBITAL);

    // Draw
    //----------------------------------------------------------------------------------
    BeginDrawing();

    ClearBackground(BLACK);

    BeginMode3D(camera);

    DrawGrid(10, 1.0f); // Draw a grid
    DrawCylinderEx((Vector3){-5, 0, 0}, (Vector3){5, 0, 0}, .1, .1, 12, RED);
    DrawCylinderEx((Vector3){0, 0, 0}, (Vector3){0, 5, 0}, .1, .1, 12, GREEN);
    DrawCylinderEx((Vector3){0, 0, -5}, (Vector3){0, 0, 5}, .1, .1, 12, BLUE);

    // draw all quadrants
    DrawMeshInstanced(my_small_sphere, matInstances, info.transform_list[0],
                      info.color_cnt);
    DrawMeshInstanced(my_small_sphere, matInstances, info.transform_list[1],
                      info.color_cnt);
    DrawMeshInstanced(my_small_sphere, matInstances, info.transform_list[2],
                      info.color_cnt);
    DrawMeshInstanced(my_small_sphere, matInstances, info.transform_list[3],
                      info.color_cnt);

    EndMode3D();

    DrawFPS(10, 10);

    Draw_Image_In_Region(target_image_tex,
                         (Rectangle){SCREEN_WIDTH - 200, 0, 200, 200});
    EndDrawing();

    // handle file dropping
    if (IsFileDropped()) {
      FilePathList files = LoadDroppedFiles();
      for (int i = 0; i < files.count; i++) {
        printf("got file dropped %s\n", files.paths[i]);
        // after this can load the file into texture
        // and sample the image
        Image test_load = LoadImage(files.paths[i]);
        Texture2D test_texture = LoadTextureFromImage(test_load);
        printf("texture id = %d\n", test_texture.id);
        if (test_texture.id == 0) {
          // invalid texture
          printf("Error unable to load %s\n", files.paths[i]);
          break;
        }
        // TODO: add free command
        free(info.transform_list);
        free(info.color_list);
        free(info.drawn_pixel_map);
        UnloadImage(target_image);
        UnloadTexture(target_image_tex);
        target_image = LoadImage(files.paths[i]);
        target_image_tex = LoadTextureFromImage(target_image);

        info = process_image(target_image);
        break;
      }
      // tells the engine we handled the files
      UnloadDroppedFiles(files);
    }
    //----------------------------------------------------------------------------------
  }
  return EXIT_SUCCESS;
}
