# Gamelogic


Items:
    Boost:
        Increases speed cap temporarily and applies boost immediately. Furthermore removes the slow effect caused by
        Lightning. Does not stack with boost pad but refreshes the effect.

    Lightning:
        Decreases speed cap temporarily and applies slow immediately. Removes boost effects.

    Reverse:
        Reverses left and right input.

    These effects are active for 5 seconds.



Colors:
    Black:
        Players gets a new item if the item slot is free.

    Blue->Yellow:
        Blue signals that the player drove over the finishing line from the correct direction. Finished Laps is
        increased by 1 if the next received color is yellow. If the backwards order (Yellow->Blue) is received, finished
        Laps is decreased by 1. This prevents driving over the finishing line, backing up and driving over the finishing
        line again.

    Green->White:
        Green signals that the player is about the leave the track. When the next received color is white, the
        speed of this player is divided by 4. This slow is reversed as soon as green is received after white.

    Red:
        Speed boost by boost pad. Same effects as Boost-item. Does do not stack, but refreshes the boost duration.


