// Copyright 2015-2016 Metric Panda. All Rights Reserved.

#include <GLFW/glfw3.h> // OpenGL
#include <stdio.h> // printf, FILE
#include <stdlib.h> // malloc, free
#define TIME_BEGIN() double TimeBegin = glfwGetTime()
#define TIME_END() double TimeEnd = glfwGetTime()
#define TIME_ELAPSED (1000.0*(TimeEnd - TimeBegin))

#define MPE_POLY2TRI_IMPLEMENTATION
#include "../MPE_fastpoly2tri.h"

internal_static
char* ReadDataFile(char* Filename)
{
  char* Result = 0;
  FILE* File = fopen(Filename, "r");
  if (File)
  {
    fseek(File, 0, SEEK_END);
    ssize_t Size = ftell(File);
    if (Size > 0)
    {
      fseek(File, 0, SEEK_SET);
      Result = (char*)malloc((size_t)Size+1);
      size_t DataRead = fread(Result, (size_t)Size, 1, File);
      if (DataRead == (size_t)Size)
      {
        Result[Size] = '\0';
      }
    }
    fclose(File);
  }
  return Result;
}

internal_static
void ShowWindow(int MaxFileCount, char** Filenames);

internal_static
MPEPolyContext LoadDataAndTriangulate(char* Filename);

internal_static
void FreeMemory(MPEPolyContext* PolyContext);

int main(int argc, char** argv)
{
  int Return = 0;
  if (argc < 2)
  {
    fprintf(stderr, "Usage: %s [-t] <filename>...\n", argv[0]);
    fprintf(stderr, "  -t to triangulate source files without opening a window\n");
    fprintf(stderr, "  <filename> is the name of the test file located in test/data\n");
    fprintf(stderr, "  can be a list of space separated filenames\n");
    Return = -1;
  }
  else
  {
    glfwInit();
    if (argv[1][0] == '-' && argv[1][1] == 't')
    {
      for (int FileIndex = 2; FileIndex < argc; ++FileIndex)
      {
        MPEPolyContext PolyContext = LoadDataAndTriangulate(argv[FileIndex]);
        FreeMemory(&PolyContext);
      }
    }
    else
    {
      ShowWindow(argc-1, argv+1);
    }
  }
}

internal_static
void FreeMemory(MPEPolyContext* PolyContext)
{
  free(PolyContext->Memory);
}

internal_static
void ResetContextForTestIteration(MPEPolyContext* PolyContext, umm IterationRollbackLocation)
{
  MPE_Assert(PolyContext->MaxPointCount);
  // Clear to zero everything except the points array in order to avoid having to 
  // parse the file between test iterations
  u8* EndOfPoints = (u8*)(PolyContext->PointsPool+PolyContext->MaxPointCount);
  umm SizeOfPoints = sizeof(MPEPolyPoint)*PolyContext->MaxPointCount;
  umm SizeToClear = PolyContext->Allocator.Size - SizeOfPoints;
  memset(EndOfPoints, 0, SizeToClear);
  memset(&PolyContext->Basin, 0, sizeof(PolyContext->Basin));
  memset(&PolyContext->EdgeEvent, 0, sizeof(PolyContext->EdgeEvent));
  PolyContext->Allocator.Used = IterationRollbackLocation;
  PolyContext->TrianglePoolCount = 0;
  PolyContext->TriangleCount = 0;
  PolyContext->PointCount = 0;
  PolyContext->NodeCount = 0;

  // Clear the linked list of edges for each point
  for (uxx PointIndex = 0; PointIndex < PolyContext->MaxPointCount; ++PointIndex)
  {
    MPEPolyPoint* Point = PolyContext->PointsPool + PointIndex;
    Point->FirstEdge = 0;
  }
}

internal_static
MPEPolyContext LoadDataAndTriangulate(char* Filename)
{
  MPEPolyContext PolyContext;
  PolyContext.Valid = 0;
  char* FileData = ReadDataFile(Filename);
  if (FileData)
  {
    u32 MaxPointCount = 0;

    //NOTE: Count the number of lines in the file.
    //      At most one point per line.
    char* At = FileData;
    do
    {
      if (*At == '\n' || *At == '\0')
      {
        MaxPointCount++;
      }
    }
    while(*At++);
    if (MaxPointCount)
    {
      bxx AddDudeHoles = 0;
      if (strstr(Filename, "dude.dat"))
      {
        AddDudeHoles = 1;
        MaxPointCount += 10;
      }
      umm MemoryRequired = MPE_PolyMemoryRequired(MaxPointCount);
      void* Memory = calloc(MemoryRequired, 1);
      if (MPE_PolyInitContext(&PolyContext, Memory, MaxPointCount))
      {
        umm IterationRollbackLocation = PolyContext.Allocator.Used;
        At = FileData;
        char* StartOfLine = At;
        do
        {
          if (*At == '\n' || (*At == '\0' && StartOfLine > At))
          {
            f64 X, Y;
            if (sscanf(StartOfLine, "%lf %lf", &X, &Y) == 2)
            {
              MPEPolyPoint* Point = MPE_PolyPushPoint(&PolyContext);
              Point->FirstEdge = 0;
              Point->X = (poly_float)X;
              Point->Y = (poly_float)Y;
            }
            StartOfLine = ++At;
          }
        }
        while(*At++);

        if (PolyContext.Valid)
        {
          uxx TriangleCount = 0;
          TIME_BEGIN();
          ResetContextForTestIteration(&PolyContext, IterationRollbackLocation);
          MPE_PolyAddEdge(&PolyContext);
          if (AddDudeHoles)
          {
            MPEPolyPoint* HeadHole = MPE_PolyPushPointArray(&PolyContext, 4);
            HeadHole[0].X = 325; HeadHole[0].Y = 437;
            HeadHole[1].X = 320; HeadHole[1].Y = 423;
            HeadHole[2].X = 329; HeadHole[2].Y = 413;
            HeadHole[3].X = 332; HeadHole[3].Y = 423;
            MPE_PolyAddHole(&PolyContext);

            MPEPolyPoint* ChestHole = MPE_PolyPushPointArray(&PolyContext, 6);
            ChestHole[0].X = (poly_float)320.72342; ChestHole[0].Y = (poly_float)480;
            ChestHole[1].X = (poly_float)338.90617; ChestHole[1].Y = (poly_float)465.96863;
            ChestHole[2].X = (poly_float)347.99754; ChestHole[2].Y = (poly_float)480.61584;
            ChestHole[3].X = (poly_float)329.8148;  ChestHole[3].Y = (poly_float)510.41534;
            ChestHole[4].X = (poly_float)339.91632; ChestHole[4].Y = (poly_float)480.11077;
            ChestHole[5].X = (poly_float)334.86556; ChestHole[5].Y = (poly_float)478.09046;
            MPE_PolyAddHole(&PolyContext);
          }
          MPE_PolyTriangulate(&PolyContext);
          TriangleCount += PolyContext.TriangleCount;
          TIME_END();
          printf("Generated %u triangles from file '%s'\n", TriangleCount, Filename);
          printf("Number of points = %u\n", PolyContext.PointCount);
          printf("--------------\n");
          printf("Completed in %0.3fms\n", TIME_ELAPSED);
        }
        else
        {
          fprintf(stderr, "Invalid file provided. No point data found\n");
        }
      }
    }
  }
  else
  {
    fprintf(stderr, "Invalid file: %s\n", Filename);
  }
  free(FileData);
  return PolyContext;
}

internal_static
void ShowWindow(int MaxFileCount, char** Filenames)
{
  int CurrentFilename = 0;
  {
    MPEPolyContext PolyContext = LoadDataAndTriangulate(Filenames[CurrentFilename]);
    GLFWwindow* Window = glfwCreateWindow(800, 600, "Fast Poly2Tri", 0, 0);
    if (Window && PolyContext.Valid)
    {
      glfwMakeContextCurrent(Window);
      glfwSwapInterval(1);

      glEnable(GL_BLEND);
      glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
      glClearColor(0.0, 0.0, 0.0, 0.0);
      glHint(GL_LINE_SMOOTH_HINT, GL_NICEST);
      glDisable(GL_DEPTH_TEST);

      glMatrixMode(GL_MODELVIEW);
      glLoadIdentity();

      f32 Scale = 1.0f;
      i32 Width = 0;
      i32 Height = 0;
      f32 PanSpeed = 5.f;
      i32 PreviousSpaceState = GLFW_RELEASE;

      for(;PolyContext.Valid;)
      {
        glfwPollEvents();
        if (glfwGetKey(Window, GLFW_KEY_ESCAPE) || glfwWindowShouldClose(Window))
        {
          break;
        }
        i32 SpaceState = glfwGetKey(Window, GLFW_KEY_SPACE);
        if (MaxFileCount > 1 && PreviousSpaceState == GLFW_PRESS && SpaceState == GLFW_RELEASE)
        {
          CurrentFilename = (CurrentFilename+1) % MaxFileCount;
          FreeMemory(&PolyContext);
          PolyContext = LoadDataAndTriangulate(Filenames[CurrentFilename]);
          if (!PolyContext.Valid)
          {
            break;
          }
        }
        PreviousSpaceState = SpaceState;

        f32 PanX = 0;
        f32 PanY = 0;
        f32 NewScale = Scale;

        if (glfwGetKey(Window, GLFW_KEY_Q))
        {
          NewScale = Scale - 0.05f;
        }
        else if (glfwGetKey(Window, GLFW_KEY_E))
        {
          NewScale = Scale + 0.05f;
        }
        if (glfwGetKey(Window, GLFW_KEY_W))
        {
          PanY = -PanSpeed;
        }
        else if (glfwGetKey(Window, GLFW_KEY_S))
        {
          PanY = PanSpeed;
        }
        if (glfwGetKey(Window, GLFW_KEY_A))
        {
          PanX = PanSpeed;
        }
        else if (glfwGetKey(Window, GLFW_KEY_D))
        {
          PanX = -PanSpeed;
        }

        i32 NewWidth, NewHeight;
        glfwGetFramebufferSize(Window, &NewWidth, &NewHeight);
        if (NewHeight != Height || NewWidth != Width || fabsf(NewScale - Scale) > 0)
        {
          Scale = NewScale;
          Width = NewWidth;
          Height = NewHeight;
          glViewport(0, 0, Width, Height);

          i32 Left = (i32)(-(f32)Width*Scale*0.5f);
          i32 Right = (i32)((f32)Width*Scale*0.5f);
          i32 Bottom = (i32)(-(f32)Height*Scale*0.5f);
          i32 Top = (i32)((f32)Height*Scale*0.5f);
          glMatrixMode(GL_PROJECTION);
          glLoadIdentity();
          glOrtho(Left, Right, Bottom, Top, 1, -1);
        }
        if (fabsf(PanY) > 0 || fabsf(PanX) > 0)
        {
          glTranslatef(-PanX, -PanY, 0);
        }

        glClear(GL_COLOR_BUFFER_BIT);
        glColor3f(1, 0, 0);
        for (uxx TriangleIndex = 0; TriangleIndex < PolyContext.TriangleCount; ++TriangleIndex)
        {
          MPEPolyTriangle* Triangle = PolyContext.Triangles[TriangleIndex];
          MPEPolyPoint* PointA = Triangle->Points[0];
          MPEPolyPoint* PointB = Triangle->Points[1];
          MPEPolyPoint* PointC = Triangle->Points[2];

          glBegin(GL_LINE_LOOP);
          glVertex2f((GLfloat)PointA->X, (GLfloat)PointA->Y);
          glVertex2f((GLfloat)PointB->X, (GLfloat)PointB->Y);
          glVertex2f((GLfloat)PointC->X, (GLfloat)PointC->Y);
          glEnd();
        }

        glfwSwapBuffers(Window);
      }
      free(PolyContext.Allocator.Memory);
    }
    else
    {
      fprintf(stderr, "Can't open glfw window\n");
      glfwTerminate();
    }
  }
}

