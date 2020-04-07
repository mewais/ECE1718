# Audio Filtering and Mixing

## How to Run?
- make
- ./project /dev/input/by-id/your-keyboard

## How to use?
- The following buttons are Piano sound keys:
  - Q, W, E, R, T, Y, U, I
  - 2, 3, 5, 6, 7
  - SPACE: piano dampening key, hold to dampen
- The following buttons can be used for recording and playing back, recording and playback works for both microphone and piano:
  - O: Toggle recording on or off, toggling on recording will toggle off playback if it's on.
  - P: Toggle playback on or off, toggling on playback will toggle off recording if it's on.
- The following buttons can be used for increasing and decreasing the volumes of the high frequency and low frequency parts, please note that the filtering works only on recorded sounds, but not piano sounds:
  - A: increase low frequency volume
  - Z: decrease low frequency volume
  - S: increase high frequency volume
  - X: decrease high frequency volume
