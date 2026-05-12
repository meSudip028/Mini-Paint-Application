# Mini-Paint-Application
Mini Paint Application using C++ and OpenGL implementing DDA, Midpoint Circle, and Flood Fill algorithms.
# Mini Paint Application 🎨

A simple paint application developed using **C++** and **OpenGL (GLUT/freeglut)** for a Computer Graphics project.

This project demonstrates the implementation of classic computer graphics algorithms such as:

- DDA Line Drawing Algorithm
- Midpoint Circle Drawing Algorithm
- Flood Fill Algorithm

---

## Features

✅ Draw Lines using DDA Algorithm  
✅ Draw Circles using Midpoint Circle Algorithm  
✅ Flood Fill Color Tool  
✅ Multiple Color Palette  
✅ Undo Functionality  
✅ Clear Canvas  
✅ Interactive GUI using OpenGL

---

## Technologies Used

- C++
- OpenGL
- GLUT / freeglut

---

## Algorithms Implemented

### 1. DDA Line Drawing Algorithm
Used to draw straight lines between two points.

### 2. Midpoint Circle Algorithm
Used for efficient circle drawing.

### 3. Flood Fill Algorithm
Used to fill enclosed areas with selected colors.

---

## Controls

| Key | Function |
|-----|----------|
| L | Line Drawing Mode |
| C | Circle Drawing Mode |
| F | Fill Color Mode |
| U | Undo Last Shape |
| X | Clear Canvas |
| ESC | Exit Application |

---

## How to Run

### Windows
1. Install OpenGL and freeglut
2. Compile using CodeBlocks/Visual Studio

### Linux
```bash
g++ paint.cpp -o paint -lGL -lGLU -lglut
./paint
