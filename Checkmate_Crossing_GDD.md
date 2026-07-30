# Checkmate Crossing

## Game Design Document

### Student Name(s)

____________________________________________________________

### Course Name

____________________________________________________________

### Date

____________________________________________________________

---

## 1. Game Overview

### Genre

Checkmate Crossing is a 3D arcade obstacle-dodging game with a medieval chess theme. The game was originally inspired by lane-crossing games, but the movement style is closer to a top-down action game such as Brawl Stars. The player can move freely around the battlefield while avoiding projectiles, area hazards, and environmental obstacles on the way to the rescue objective.

### Target Platform

The target platform for the prototype is PC. The game is designed to run as an OpenGL application using keyboard controls.

### Target Audience

The target audience is casual players, students, and players who enjoy simple but challenging arcade games. The game is easy to understand because the main objective is simply to move from the starting area to the finish, but it also has room for more advanced mechanics through collectible chess-piece abilities.

### Brief Summary

In Checkmate Crossing, the player begins as a pawn who must travel across a dangerous battlefield to rescue the captured king. The battlefield contains moving and stationary hazards such as arrows, spears, cannonballs, fireballs, lightning, rolling rocks, rolling logs, spikes, mud, fencing, walls, palisades, trees, rocks, bushes, and cows.

As the pawn progresses, it can collect allied chess pieces. These allies allow the pawn to temporarily summon or activate special abilities based on different chess pieces. For example, bishops can remove obstacles, knights can provide movement speed and collision immunity, rooks can grant a shield, and queens can combine multiple abilities for a short time. The main prototype focuses on the core game loop first: movement, obstacles, collisions, health, checkpoints, and reaching the king.

### Main Gameplay Objective

The main objective is to guide the pawn from the starting point to the king's cage at the end of the level. The player must avoid incoming attacks, survive with limited health, move through or around environmental hazards, use checkpoints, and eventually reach the king to free him. When the pawn reaches the king, the cage opens and the level is completed.

---

## 2. Gameplay

### Core Gameplay Mechanic

The core gameplay mechanic is moving a pawn through a dangerous battlefield while avoiding obstacles and hazard zones. The player controls the pawn using free top-down movement instead of moving one tile or lane at a time. This makes the game feel closer to Brawl Stars, where the player constantly adjusts position, dodges attacks, and finds openings.

The game is designed around a simple loop:

1. The player starts at the beginning of the level as a pawn.
2. The player moves freely forward, backward, left, and right.
3. Moving hazards travel across the battlefield or create dangerous area effects.
4. Stationary hazards block, slow, damage, or redirect the player.
5. If the player collides with hazards, they lose health or suffer another penalty.
6. Checkpoints allow the player to recover progress after taking too much damage.
7. The player reaches the king's cage to win the level.

For the prototype, the most important mechanic is the obstacle avoidance system. Chess abilities are planned as an expanded feature after the basic version is playable.

### Player Controls

The player uses keyboard input to control the pawn.

| Input | Action |
| --- | --- |
| W or Up Arrow | Move forward |
| S or Down Arrow | Move backward |
| A or Left Arrow | Move left |
| D or Right Arrow | Move right |
| Space | Activate stored chess ability, if available |

The camera is fixed above the battlefield and follows the pawn as it moves through the level. This gives the player a clear view of incoming projectiles, nearby hazards, and the path toward the king.

### Moving Obstacles

Moving obstacles are hazards that travel through the battlefield or create temporary danger zones. These obstacles make the player dodge, reposition, and react quickly.

| Obstacle | Behavior |
| --- | --- |
| Arrows | Slow horizontal projectiles that can enter from the left or right. Their range covers the full horizontal width of the map. |
| Spears | Similar speed to arrows, but they travel in a curved trajectory and create a small area effect where they land or pass through. |
| Cannonballs | Faster than arrows, deal more damage, and explode with an area effect. Their range is shorter and reaches about 70% of the map's width. |
| Fireballs | Similar to spears, but with a larger impact area. They leave fire behind that deals continuous damage over time while the player remains inside it. |
| Lightning | An unavoidable strike if the player stays in a marked danger area for too long, such as standing near a tree for several seconds. The warning area gives the player a chance to move away. |
| Rolling Rock | A slow vertical projectile with a large hit area. It is easier to see coming but harder to move around because of its size. |
| Rolling Log | Similar to the rolling rock, but faster and with a smaller hit area. It pressures the player to react quickly. |

### Stationary Hazards

Stationary hazards shape the environment and influence how the player moves through the battlefield.

| Hazard | Behavior |
| --- | --- |
| Spikes | Damage the player and knock them backward when touched. |
| Mud | Slows the player while walking through it. The slow effect remains for a few seconds after leaving the mud. |
| Fencing | Blocks the player, but can be jumped over. It can also be broken by bishop and queen abilities. |
| Walls | Completely block the player. They cannot be jumped over and cannot be broken by bishop or queen abilities. |
| Palisade | Blocks the player, but can be jumped over or broken by bishop and queen abilities. Jumping over it damages the player. |
| Trees | Completely block the player and may also be used as danger zones for lightning attacks. |
| Rocks | Block movement, but can be jumped over. |
| Bushes | Slow the player only while the player is inside the bush. |
| Cow | A moving environmental hazard that follows the player and interferes with movement. |

### Game Objectives

The main objectives are:

- Move from the starting area to the king's cage.
- Avoid moving hazards such as arrows, spears, cannonballs, fireballs, lightning, rolling rocks, and rolling logs.
- Avoid or navigate around stationary hazards such as spikes, mud, fencing, walls, palisades, trees, rocks, bushes, and cows.
- Reach checkpoints placed throughout the map.
- Survive with limited health.
- Rescue the king at the end of the level.

Optional extended objectives include:

- Collect chess-piece allies.
- Use each chess ability at the correct time.
- Complete the level with as much health remaining as possible.
- Finish the level faster by taking riskier paths.

### Win and Lose Conditions

The player wins by reaching the king's cage at the end of the level. When the pawn reaches the king, the cage opens, the king is rescued, and the characters celebrate.

The player loses progress by taking damage from obstacles. The pawn has five health points. Each collision with a damaging hazard removes one health point. To prevent the player from losing all health instantly from one collision, the player receives a short damage cooldown after being hit.

When the pawn loses all five health points, the pawn respawns at the latest checkpoint. If the player has not reached a checkpoint yet, the pawn respawns at the beginning of the level.

For the prototype, the game does not require a permanent game-over screen. Instead, the player is encouraged to keep trying from the most recent checkpoint until they reach the king.

### Progression and Scoring System

The main form of progression is distance through the level. The map is divided into lanes and checkpoint zones. Every major section becomes more difficult than the previous one by introducing faster obstacles, tighter timing windows, or more complicated object placement.

A full scoring system is optional, but possible scoring ideas include:

- Time taken to rescue the king.
- Health remaining at the end of the level.
- Number of collisions taken.
- Number of chess-piece abilities collected.
- Number of checkpoints reached.

For the prototype, progress is measured through checkpoints and completion of the level rather than a numerical score.

---

## 3. Story and Setting

### Setting

Checkmate Crossing takes place on a fantasy chess battlefield. The land resembles a large outdoor chessboard mixed with a medieval battlefield. The ground is dark green, with open movement spaces, hazard zones, and environmental blockers. The environment includes castle-like barriers, wooden palisades, trees, spikes, mud, rocks, bushes, cows, and battlefield projectiles.

The level starts in a relatively safe area where the pawn prepares to begin the rescue mission. As the pawn moves forward, the battlefield becomes more dangerous. The final area contains the captured king locked in a cage.

### Characters

The main character is the pawn. The pawn represents a small but brave chess piece trying to save the king. Although the pawn is one of the weakest pieces in traditional chess, in this game it becomes the hero.

The king is the rescue target. He is trapped in a cage at the end of the level. Reaching the king completes the main objective.

Other chess pieces appear as collectible allies or temporary summons. These include:

- Bishop: Removes two nearby obstacles.
- Knight: Gives temporary collision immunity and increased movement speed.
- Rook: Gives the player a shield that blocks one hit.
- Queen: Combines multiple abilities for a short duration.

### Narrative Background

The king has been captured and locked away at the far end of a dangerous battlefield. The pawn must cross enemy-controlled lanes filled with traps and projectiles to reach the cage. Along the way, allied chess pieces help the pawn by granting temporary powers.

The story is simple so that the focus remains on gameplay. The rescue mission gives the player a clear reason to move forward and makes the finish line more meaningful than simply reaching the end of a map.

### Theme and Atmosphere

The main theme is bravery against overwhelming odds. The pawn is small compared to the dangers around it, but with careful movement, timing, and help from allies, it can still rescue the king.

The atmosphere should feel playful, strategic, and slightly intense. The chess theme gives the game a recognizable visual identity, while the obstacle-crossing gameplay keeps the experience fast and arcade-like.

---

## 4. Level Design

### Environment Description

The game environment is built as a top-down battlefield with several themed sections. The player starts at one end of the map and moves forward toward the king's cage, but they are free to move in any direction within the playable area. Each section has a different hazard pattern. Some areas are open and safe, while others contain moving obstacles, stationary hazards, blocked paths, or temporary danger zones.

The level is designed to teach the player gradually. Early areas have slower and simpler obstacles. Later areas introduce faster projectiles, curved attacks, explosions, fire damage, lightning warnings, and environmental blockers. Checkpoints are placed approximately every 20 units so the player does not have to restart the entire level after losing all health.

### Basic Scene Layout

```text
Start Area
    |
    v
Safe Grass Lane
    |
    v
Arrow Field
    |
    v
Safe Grass Lane
    |
    v
Spike and Mud Field
    |
    v
Checkpoint
    |
    v
Cannonball Field
    |
    v
Fence, Tree, and Rock Field
    |
    v
Fireball and Lightning Field
    |
    v
Final Safe Area
    |
    v
King's Cage
```

### Object Placement

The start area contains the pawn and gives the player room to learn movement. The first hazard field introduces a moving projectile, such as an arrow moving from one side of the screen to the other. A safe area follows so the player can pause and plan.

Stationary hazards such as spikes, mud, fences, walls, palisades, trees, rocks, bushes, and cows are placed to shape the path and force the player to make movement decisions. Moving hazards such as cannonballs, fireballs, rolling rocks, and rolling logs create timing challenges.

The checkpoint area is placed after the player completes the first group of hazards. This rewards progress and reduces frustration. The final section contains more difficult obstacles before opening into the king's cage area.

### Simple Top-Down Map

```text
            KING'S CAGE
        [ Captured King ]
================================
 Final Safe Area
================================
 Lightning / Fireball Field
 <---- hazard movement ---->
================================
 Fence / Tree / Rock Obstacles
 [T]     [F]       [R]     [W]
================================
 Cannonball Field
 ----> cannonballs ---->
================================
CHECKPOINT
================================
 Spike / Mud / Bush Field
 [S] [M]     [S]     [M] [S]
================================
Safe Grass Lane
================================
 Arrow Field
 <---- arrows ---->
================================
 Safe Grass Lane
================================
 START AREA
        [ Pawn ]
```

### Level Design Goals

The level should:

- Be easy to understand from the camera view.
- Make obstacle patterns readable.
- Give the player safe spaces between difficult hazard fields.
- Introduce hazards one at a time before combining them.
- Use checkpoints to support repeated attempts.
- End with a clear visual goal: the king's cage.

---

## 5. Technical Implementation

### OpenGL Rendering Pipeline

The prototype uses the OpenGL rendering pipeline to draw the game world. The application sends vertex data to the GPU, processes it through shaders, and displays 3D objects such as the player pawn, ground plane, obstacles, and king cage.

The general rendering process is:

1. Create vertex data for objects.
2. Store the vertex data in GPU buffers.
3. Configure vertex attributes.
4. Use GLSL shaders to process vertices and fragments.
5. Apply transformations for object position, rotation, and scale.
6. Render the scene each frame.
7. Update object positions and repeat the process in the game loop.

This structure allows the game to update moving obstacles, player movement, and camera position in real time.

### VAO, VBO, and EBO

The project uses VAOs, VBOs, and EBOs to manage geometry.

The VBO stores vertex data such as positions, texture coordinates, and normals. The EBO stores index data, allowing the game to reuse vertices when drawing objects like cubes or planes. The VAO stores the configuration of the vertex attributes so OpenGL knows how to interpret the vertex data.

For example, a cube obstacle can use a VBO for its vertex positions and texture coordinates, an EBO for its triangle indices, and a VAO to connect the data layout to the shader inputs.

### GLSL Shaders

The game uses GLSL shaders to control how objects appear on screen. The vertex shader receives vertex positions and transforms them using model, view, and projection matrices. The fragment shader determines the final color or texture of each pixel.

The prototype can use simple shaders for colored objects at first. For example:

- White or light-colored cube for the pawn placeholder.
- Red or dark-colored cubes for moving hazards.
- Yellow or gold cube for the king placeholder.
- Dark green ground plane for the battlefield.

As the project develops, textures and lighting can be added to make the chess battlefield more visually polished.

### Object Transformations

Object transformations are used to place, move, rotate, and scale objects in the world. Each object has a model matrix that transforms it from local space to world space.

The pawn uses translation to move freely across the level. Obstacles use translation to move horizontally, vertically, or along curved paths. Stationary objects such as trees, fences, spikes, walls, palisades, rocks, and bushes use translation and scaling to create the battlefield layout.

The main transformations used are:

- Translation for position.
- Rotation for object direction or visual animation.
- Scaling for object size.

For example, the ground plane is scaled to cover the level, while obstacles are translated each frame based on their speed and direction. Some hazards also use timed area effects, such as cannonball explosions, fire left behind by fireballs, or lightning warning zones.

### Camera System

The game uses a fixed follow camera. The camera is positioned above the pawn and angled slightly forward toward the upcoming battlefield. This gives the player a clear view of the level while keeping the controls simple.

A possible camera setup is:

```text
Camera position = player position + (0, 12, 10)
Camera target = player position + (0, 0, -5)
```

This creates a top-down angled view similar to a Brawl Stars-style action game. The player should not need to rotate the camera during gameplay.

### Projection

The game uses perspective projection to create a 3D view of the battlefield. Perspective projection makes objects farther from the camera appear smaller, giving the level depth.

The projection matrix works together with the view matrix and model matrix:

```text
Final position = projection * view * model * vertex position
```

This is one of the main concepts used in the OpenGL rendering pipeline. It allows the game to position objects in a 3D world and display them correctly on the 2D screen.

### Texture Mapping

Texture mapping can be used to improve the appearance of objects. The ground may use a dark green grass texture or a subtle battlefield pattern. Obstacles can use textures such as wood for fences, metal for spikes, stone for walls, or glowing effects for fireballs and lightning.

Each textured object needs texture coordinates in its vertex data. These coordinates tell the fragment shader which part of the image texture should appear on each part of the model.

In the early prototype, simple colors may be used first. Textures can be added after the movement, collision, and win condition are working.

### Lighting

Lighting is used to make the 3D scene easier to read and more visually interesting. The prototype can use a simple directional light to simulate sunlight over the battlefield. Ambient lighting can be included so shadows are not too dark.

Possible lighting components include:

- Ambient light for general visibility.
- Diffuse light to show object shape.
- Specular light for highlights on polished chess pieces.

Lighting is especially useful once the placeholder cubes are replaced with chess-piece models.

### Collision Detection

Collision detection is used to determine when the pawn touches hazards, checkpoints, collectibles, or the king's cage. The prototype uses simple AABB collision because it is efficient and appropriate for box-shaped gameplay objects. For circular area effects such as explosions, fire zones, and lightning warning areas, the game can also use distance-based collision checks.

Each object has a bounding box based on its position and size. The game checks whether the pawn's bounding box overlaps with another object's bounding box.

Collision is used for:

- Taking damage from arrows, spears, cannonballs, fireballs, or lightning.
- Taking damage or knockback from spikes.
- Applying slowing effects from mud and bushes.
- Blocking movement with fencing, walls, palisades, trees, and rocks.
- Tracking area effects such as explosions, fire damage over time, and lightning strikes.
- Activating checkpoints.
- Collecting chess-piece ability tokens.
- Detecting when the pawn reaches the king.

To avoid repeated damage from a single collision, the player has a short damage cooldown after being hit. This prevents one obstacle from removing all five health points immediately.

### Prototype Development Plan

The prototype was planned around building the smallest playable version first. The first version focuses on proving the main game loop before adding advanced chess abilities or detailed art.

The prototype development order is:

1. Render a dark green ground plane.
2. Render a temporary cube as the pawn.
3. Add keyboard movement.
4. Add a fixed follow camera.
5. Add one moving horizontal arrow obstacle.
6. Add collision between the pawn and obstacle.
7. Add health and damage cooldown.
8. Add a checkpoint.
9. Add the king cage and win condition.
10. Add additional hazards such as spikes, mud, cannonballs, and fireballs.
11. Add collectible chess-piece abilities if time allows.

This approach makes sure the game is playable early. Once the basic systems work, the placeholder shapes can be replaced with chess models, textures, lighting, and improved level design.

---

## 6. Conclusion

Checkmate Crossing is a 3D arcade obstacle-dodging game with a chess rescue theme. The player controls a pawn trying to cross a dangerous battlefield and rescue the captured king. The design combines free top-down movement, readable obstacle patterns, health, checkpoints, environmental hazards, and optional chess-piece abilities.

The prototype applies important OpenGL concepts such as VAOs, VBOs, EBOs, shaders, transformations, projection, camera movement, texture mapping, lighting, and collision detection. By focusing first on a small playable version, the project can demonstrate both technical course concepts and a clear, complete game idea.
