#!/bin/bash

# Usage: ./add_assertions.sh <csv_file> <rec_file>

CSV_FILE="$1"
REC_FILE="$2"

assert_if_changed() {
    local tick="$1"
    local har="$2"
    local prop="$3"
    local current="$4"
    local prev="$5"
 
    # disable xpos assertions for now, they're too noisy
    if [ "$current" != "$prev" -a "$prop" != "xpos" ]; then
        echo "$tick $prop $current"
        # tick offset is 11, for some reason???
        rectool -f "$REC_FILE" --assert --assertion_tick="$(($tick + 11))" --operator=eq \
            --operand1="$har" --operand1_value="$prop" \
            --operand2=literal --operand2_value="$current" \
            -o "$REC_FILE"
    fi
}

# Initialize previous values
declare -A prev_values=(
    [p1_hp]="" [p1_anim]="" [p1_xpos]="" [p1_ypos]="" [p1_xvel]="" [p1_yvel]=""
    [p2_hp]="" [p2_anim]="" [p2_xpos]="" [p2_ypos]="" [p2_xvel]="" [p2_yvel]=""
)

# Process CSV
{
    read -r header  # Skip header
    while IFS=, read -r tick \
        p1_hp p1_anim p1_xpos p1_ypos p1_xvel p1_yvel \
        p2_hp p2_anim p2_xpos p2_ypos p2_xvel p2_yvel
    do
        # Har1 assertions
        assert_if_changed "$tick" har1 health "$p1_hp" "${prev_values[p1_hp]}"
        assert_if_changed "$tick" har1 anim "$p1_anim" "${prev_values[p1_anim]}"
        assert_if_changed "$tick" har1 xpos "$p1_xpos" "${prev_values[p1_xpos]}"
        assert_if_changed "$tick" har1 ypos "$p1_ypos" "${prev_values[p1_ypos]}"
        assert_if_changed "$tick" har1 xvel "$p1_xvel" "${prev_values[p1_xvel]}"
        assert_if_changed "$tick" har1 yvel "$p1_yvel" "${prev_values[p1_yvel]}"

        # Har2 assertions
        assert_if_changed "$tick" har2 health "$p2_hp" "${prev_values[p2_hp]}"
        assert_if_changed "$tick" har2 anim "$p2_anim" "${prev_values[p2_anim]}"
        assert_if_changed "$tick" har2 xpos "$p2_xpos" "${prev_values[p2_xpos]}"
        assert_if_changed "$tick" har2 ypos "$p2_ypos" "${prev_values[p2_ypos]}"
        assert_if_changed "$tick" har2 xvel "$p2_xvel" "${prev_values[p2_xvel]}"
        assert_if_changed "$tick" har2 yvel "$p2_yvel" "${prev_values[p2_yvel]}"

        # Update previous values
        prev_values=(
            [p1_hp]="$p1_hp" [p1_anim]="$p1_anim"
            [p1_xpos]="$p1_xpos" [p1_ypos]="$p1_ypos"
            [p1_xvel]="$p1_xvel" [p1_yvel]="$p1_yvel"
            [p2_hp]="$p2_hp" [p2_anim]="$p2_anim"
            [p2_xpos]="$p2_xpos" [p2_ypos]="$p2_ypos"
            [p2_xvel]="$p2_xvel" [p2_yvel]="$p2_yvel"
        )
    done
} < "$CSV_FILE"
