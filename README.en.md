# Final Chase

[中文](README.md) | English

Competitive Parkour Tag Game Inspired by World Chase Tag, developed with Unreal Engine 5.7

## Quick Start
```bash
git lfs install
git clone https://github.com/ar0nkk/wct-ue5.git
```
To test the 2-player game on a single device, open the project and click the three dots next to the **Play** button (Play Settings), then:
- **Number of Players** -> `2`
- **Net Mode** -> **Play as Listen Server**

## Rules
A 1v1 chase game (Chaser vs Runner)

- **Match Structure**: 8 rounds per match.
- **Round Flow**:
  - **Pre-round (8s)**: Runner can move freely to position; Chaser cannot move but can rotate the camera.
  - **Chase (20s)**: Chaser regains movement and attempts to tag the Runner.
- **Win Conditions**:
  - **Chaser**: Tag the Runner within 20 seconds; the round ends immediately but no score is awarded.
  - **Runner**: Survive for 20 seconds; the Runner wins and gains 1 point.
- **Role Swap**: After each round, the winner becomes the **Runner** in the next round. After 8 rounds, the higher score wins the match.

## Controls (Updating...)
- LAN multiplayer: Host enters the game, client connects by entering the host IP.
- **WASD**: Move
- **Space**: Jump
- **Shift**: Sprint
- **Left Click**: Dive / tag attempt (animation TBD)
- **C**: Slow = crouch; fast = slide
- **Ctrl**: Walk

## Features
- **Action system (GAS-driven)**: Built on Epic's Gameplay Ability System (GAS).
- **Dive**: The Chaser can dive to attempt a tag. Hit validation is server-authoritative (collision + role checks).
- **Stamina (WIP)**:
  - Each character has its own stamina bar.
  - **Cost**: Sprint, Jump, Dive, etc. consume stamina.
  - **Recovery**: Stamina regenerates slowly when not performing high-cost actions.

---

## TODO
### High Priority
- [ ] Dive and other character actions
- [ ] Level/arena setup
- [ ] More interaction animations with obstacles
- [ ] Stamina system or action cooldowns
- [ ] Other balancing changes

### Low Priority
- [ ] Round-end sound cues
- [ ] UI polish
- [ ] Game settings (keybinds, etc.)
- [ ] Spectator/VFX
