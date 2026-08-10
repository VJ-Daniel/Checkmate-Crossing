# Checkmate Crossing
## Game Design Document

### Collaborators
VJ Daniel Uy  
Liyyu Satuhati  
John Templonuevo  
Ayub Abdelgawad  
Kaung Khant San  

### Course
GAM531NSA - Game Engine Foundations

### Instructor
Prof. Leonardo Moura

---

## Table of Contents

1. Introduction
2. Gameplay Summary
3. Story and World
4. Gameplay Design
5. Design Vision and Core Pillars
6. Intended Player Experience
7. Characters and Chess Abilities
8. Level and Environment Design
9. Progression and Replayability
10. Target Audience and Platform
11. Technical Design

---

## Introduction
*Checkmate Crossing* is a 3D arcade obstacle-dodging game set on a medieval fantasy battlefield inspired by chess. The player controls a pawn attempting to cross enemy territory and rescue the captured king. Although the game takes visual inspiration from lane-crossing games, movement is not restricted to fixed lanes. Instead, the player can move freely across the battlefield using top-down action controls.

The game combines simple movements with hazards that require timing, positioning, and quick reactions. Projectiles and environmental obstacles shape the available route, and temporary chess-piece abilities allow the player to survive difficult sections. The concept is designed to be immediately understandable while still offering enough variety to support increasingly difficult obstacle patterns.

---

## Gameplay Summary

**Working Title:** Checkmate Crossing  
**Genre:** 3D Arcade, Obstacle-Dodging, Action  
**Mode:** Single-Player  
**Platform:** PC  
**Technology:** C++ and OpenGL  

The player begins each level as a pawn at one end of a large battlefield, while the king is imprisoned in a cage at the opposite end. To complete the level, the player must move through the field while avoiding projectiles, environmental hazards, blocked paths, and timed danger zones without losing all of their health.

The pawn starts with five health points, and taking damage removes one health point. A brief invulnerability period prevents one obstacle from dealing repeated damage immediately. If the player loses all their health points, the player is sent back to the latest checkpoint rather than restarting the entire level. The level is completed when the pawn reaches the king and opens the cage.

Collectible allied chess pieces provide temporary power-ups. These powers are intended to give the player different ways to respond to hazards instead of relying only on movement.

---

## Story and World

### Narrative Premise

The king has been captured and taken across a dangerous battlefield controlled by an enemy army. The kingdom's stronger pieces have been scattered, leaving a single pawn to embark on the rescue. During the journey, the pawn finds allied chess pieces that temporarily lend their abilities. With their help, the pawn must survive the battlefield and reach the king.

Although considered to be the weakest in the kingdom, will this pawn be able to save the king and become the hero?

### World and Setting

The game takes place in a stylized medieval environment built around chess imagery. The battlefield resembles a large outdoor board made of dark green ground, stone structures, wooden defenses, and open combat lanes. Castle walls and palisades reinforce the setting, while the chess-piece characters give the world a playful and recognizable identity.

### Main Themes

- Bravery despite being underestimated
- Progress through persistence
- Strategy under pressure
- Cooperation between different chess pieces
- Rescue and loyalty

---

## Gameplay Design

### Core Gameplay Loop

Each attempt follows the same basic loop:

1. The pawn begins at the start of the battlefield or the most recent checkpoint.
2. The player moves freely in four directions while observing upcoming hazards.
3. Moving projectiles and environmental obstacles force the player to dodge, wait, or change direction.
4. The player may collect a chess-piece ability and activate it when needed.
5. Taking damage reduces health and briefly activates damage immunity.
6. Losing all health returns the pawn to the latest checkpoint.
7. Reaching the king's cage completes the level.

The prototype focuses first on movement, collision, health, checkpoints, and the rescue objectives. Chess abilities and more complex hazards are added after the basic loop is stable.

### Moment-to-Moment Actions

During play, the player continuously makes small movement decisions:

- Move forward to make progress towards the king
- Move sideways to avoid crossing projectiles
- Move backwards when a path becomes unsafe
- Wait in a safe area for an obstacle pattern to pass
- Navigate around walls, trees, rocks, and fences
- Cross puddles or bushes at the cost of reduced movement speed
- Activate a stored chess ability at the right moment
- Reach checkpoints to secure progress

### Controls and Input Methods

| Input | Action |
|---|---|
| W / Up Arrow | Move forward |
| S / Down Arrow | Move backward |
| A / Left Arrow | Move left |
| D / Right Arrow | Move right |
| Space | Jump |
| E | Interact with doors or activate stored chess ability |

### Gameplay Rhythm

The level alternates between pressure and recovery. Hazard fields create short periods of intense movement, while safe grass lanes give the player time to reposition and prepare for the next challenge. Early sections use slower and more predictable hazards, while later sections reduce the amount of safe spaces and combine multiple obstacle types.

This rhythm prevents the experience from becoming exhausting while still allowing the difficulty to rise steadily.

### Player Goals

The immediate goal is to survive each hazard section and continue moving towards the king. Secondary goals include:

- Reaching every checkpoint
- Avoiding damage and finishing with as much remaining health as possible
- Completing the level quickly
- Collecting and successfully using chess-piece abilities

### Win and Failure Conditions

The player wins when the pawn reaches the king's cage. The cage opens, the king is rescued, abd the level ends with a short fanfare.

The pawn has five health points. Damage is caused by projectiles, hazards, and other dangers such as spikes, explosions, or fire. When health reaches zero, the pawn respawns at the latest reached checkpoint. Before reaching the first checkpoint, the pawn respawns at the starting area.

---

## Design Vision and Core Pillars

### Design Goals

The main design goal is to create a readable and responsive obstacle-dodging game that is easy to understand but challenging to complete. The player should always know where they are going, what is dangerous, and why they took damage.

The game should also use its chess theme mechanically rather than only visually. Each collectible piece should provide an ability related to the traditional identity of that chess piece.

### Core Design Pillars

#### 1. Readable Hazards

Dangerous objects must have clear shapes, movement directions, and warning cues. Players should be able to understand a threat quickly enough to react fairly.

#### 2. Responsive Free Movement

The pawn should respond immediately to input. Movement must feel precise because avoiding obstacles depends on small positional adjustments.

#### 3. Increasing Battlefield Pressure

Difficulty should rise through faster hazards, overlapping patterns, narrower routes, and more complicated combinations rather than through unclear or unpredictable damage.

#### 4. Chess-Inspired Abilities

The chess theme should influence gameplay. Bishops, knights, rooks, and queens provide powers that reflect their role and importance within chess.

#### 5. Frequent Recovery Points

Checkpoints and safe lanes reduce frustration and encourage the player to keep attempting difficult sections.

---

## Intended Player Experience

Players should experience:

- Tension while waiting for an opening between hazards.
- Satisfaction after crossing a difficult obstacle field.
- Urgency when health is low or several attacks overlap.
- Relief when reaching a checkpoint or safe lane.
- Empowerment when activating a chess ability.
- Achievement when the pawn finally reaches the king.

The overall tone should feel adventurous and playful rather than violent or frightening. The battlefield is dangerous, but the stylized chess characters and block-based art direction keep the experience approachable.

---

## Characters and Chess Abilities

### Playable Character: The Pawn

The pawn is the main playable character. It is physically smaller and less powerful than the other chess pieces, but it is determined enough to cross the battlefield and rescue the king.

**Personality:**

- Brave
- Persistent
- Loyal
- Resourceful

**Gameplay Role:**

- Controlled directly by the player
- Moves freely in four directions
- Has five health points
- Activates collected chess abilities
- Respawns at checkpoints after losing all health

**Visual Design:**

The pawn is a small block-based medieval soldier with light armor, dark blue cloth, brown leather equipment, and a simple helmet. Its smaller size helps visually separate it from stronger chess pieces such as the rook, queen, and king.

### The King

The king is the rescue target and appears inside a cage at the end of the level. He does not participate directly in gameplay during the prototype. Reaching him activates the win condition.

### Allied Chess Pieces

#### Bishop

**Gameplay Function:** Removes up to two nearby breakable obstacles.

The bishop is useful when fences or palisades block the safest route. It cannot remove solid walls or other permanent level boundaries.

#### Knight

**Gameplay Function:** Temporarily increases movement speed and grants collision immunity.

The knight allows the player to move rapidly through a dangerous section or recover after becoming trapped by overlapping hazards.

#### Rook

**Gameplay Function:** Grants a shield that blocks one damaging hit.

The shield disappears after absorbing damage. It provides protection but does not remove slowing effects from puddles or bushes.

#### Queen

**Gameplay Function:** Temporarily combines several allied abilities.

The queen is the rarest and strongest collectible. Her power may provide increased speed, temporary protection, and the ability to destroy breakable obstacles for a limited duration.

---

## Level and Environment Design

### Environment Structure

The level is a long battlefield divided into themed sections. The player generally progresses toward the far end of the map but remains free to move in any direction within the playable boundaries.

A possible progression is:

```text
Start Area
    ↓
Safe Grass Lane
    ↓
Arrow Field
    ↓
Safe Grass Lane
    ↓
Spike, Mud, and Bush Field
    ↓
Checkpoint
    ↓
Cannonball Field
    ↓
Fence, Tree, Wall, and Rock Section
    ↓
Fireball and Lightning Field
    ↓
Final Safe Area
    ↓
King's Cage
```

### Moving Hazards

| Hazard | Gameplay Behaviour |
|---|---|
| Arrows | Slow horizontal projectiles that cross the full width of the battlefield. |
| Spears | Curved projectiles that create a small danger area near their path or landing point. |
| Cannonballs | Fast projectiles that travel a shorter distance and explode on impact. |
| Fireballs | Projectiles that create a larger impact zone and leave damaging fire behind. |
| Lightning | A timed strike that targets a marked area after a visible warning period. |
| Rolling Rocks | Large, slow-moving hazards with wide collision areas. |
| Rolling Logs | Faster hazards with narrower collision areas than rolling rocks. |

### Stationary and Environmental Hazards

| Hazard | Gameplay Behaviour |
|---|---|
| Spikes | Deal damage and knock the pawn backward. |
| Puddle | Slows the pawn while inside and briefly after leaving. |
| Fencing | Blocks movement but may be jumped over or destroyed by certain abilities. |
| Walls | Permanently block movement and cannot be destroyed. |
| Palisades | Block movement, may be destroyed, and cause damage if crossed directly. |
| Trees | Permanently block movement and may act as lightning danger locations. |
| Rocks | Block movement but may be crossed or avoided depending on implementation. |
| Bushes | Reduce movement speed only while the pawn remains inside them. |
| Cows | Move through the environment and interfere with the player's route. |

### Level Design Goals

The level should:

- Introduce one hazard at a time before combining hazards.
- Keep dangerous movement patterns visible from the fixed camera.
- Provide safe areas between difficult sections.
- Avoid creating unavoidable damage situations.
- Use obstacles to create multiple possible routes.
- Place checkpoints after meaningful accomplishments.
- Keep the king's cage recognizable as the final destination.

---

## Progression and Replayability

### Player Progression

Progress within the prototype is mainly measured by distance and checkpoint completion. Each checkpoint represents advancement through a major section of the battlefield.

If the project is expanded, progression may include:

- Additional levels with new battlefield layouts.
- More complex combinations of hazards.
- Unlockable visual variations for the pawn.
- Improved or alternate chess-piece abilities.
- Timed challenges and optional objectives.

### Replayability

Replayability comes from improving completion time, reducing damage, trying different routes, and deciding when to use limited abilities. Small changes to hazard timing or placement could make repeated attempts less predictable without changing the overall level structure.

---

## Target Audience and Platform

### Age Range and Player Profile

The game is intended for casual players who enjoy short action challenges, simple controls, and stylized 3D games. It is suitable for players who may not be familiar with chess because knowledge of chess rules is not required.

The likely audience includes:

- Casual players who prefer understandable objectives.
- Players who enjoy obstacle-dodging and reaction-based games.
- Players attracted to medieval or chess-inspired themes.
- Players interested in short PC games.
- Players who enjoy improving times and mastering obstacle patterns.

### Platform Compatibility

The prototype is designed for PC using keyboard controls. The game is built in C++ with OpenGL and is intended to run as a desktop application.

A stable frame rate is important because the player must react to moving hazards. The block-based visual style and reusable meshes help keep the rendering requirements appropriate for a student prototype.

---

## Technical Design

### Rendering Pipeline

The game uses OpenGL to render the battlefield, chess-piece models, obstacles, and environmental props. Geometry is sent to the GPU through vertex and index buffers, then processed using GLSL shaders.

The main rendering process is:

1. Create or load mesh geometry.
2. Store vertex data in VBOs.
3. Store reusable indices in EBOs where required.
4. Configure vertex attributes through VAOs.
5. Apply model, view, and projection matrices.
6. Process lighting and colour in shaders.
7. Render the scene each frame.

### Chess-Piece Models

The chess pieces use a low-poly, block-based style. Humanoid pieces share common body components such as feet, legs, torso, arms, head, and equipment. Individual piece types are distinguished by their size, headwear, clothing, and props.

Reusing shared geometry keeps the models visually consistent and reduces unnecessary duplication.

### Camera System

The game uses a fixed follow camera positioned above and behind the pawn. The camera follows the player's progress while looking toward the upcoming battlefield.

A representative setup is:

```text
Camera position = player position + (0, 12, 10)
Camera target   = player position + (0, 0, -5)
```

The player does not manually rotate the camera. This keeps the controls simple and ensures hazards appear from a consistent viewpoint.

### Transformations and Projection

Objects use translation, rotation, and scaling to place them in the game world. Perspective projection gives the battlefield visible depth while preserving a readable top-down angle.

```text
Final vertex position = projection × view × model × local vertex position
```

### Lighting and Colour

A directional light simulates sunlight across the battlefield. Ambient lighting prevents shadowed sides from becoming unreadable. Diffuse lighting helps reveal the form of the block-based models, while limited specular highlights can be used on metal armor and weapons.

The visual palette uses:

- Off-white and silver armor
- Dark blue cloth and capes
- Brown leather, boots, saddles, and wood
- Gold trim for royal or religious pieces
- Dark green ground and environment colours

### Collision Detection

AABB collision detection is used for box-shaped characters and obstacles. Distance-based checks may be used for circular or radial effects such as explosions, fire zones, and lightning areas.

Collision detection supports:

- Damage from projectiles and hazards
- Blocking movement
- Slowing terrain
- Checkpoint activation
- Ability collection
- Shield protection
- Reaching the king's cage

### Health and Damage Cooldown

The pawn begins with five health points. When a damaging collision occurs, health decreases and a short cooldown prevents repeated damage from the same overlap. During the cooldown, the pawn may flash to indicate temporary protection.

### Checkpoint System

Each checkpoint records a respawn location. When the pawn loses all health, its position and health reset using the latest activated checkpoint. This system allows difficult sections to be repeated without forcing the player to replay the entire level.

---
