#!/usr/bin/env bash

size=960x540
quality=90
mkdir scaled
mkdir originals

for img in *; do
    if [[ "$img" != "$0"] || ["$img" != "scaled" ]]; then
        output="scaled/${img%%.*}.jpg"
        test=`convert $img -format "%[fx:(w/h>1)?1:0]" info:`
        if [ $test -eq 0 ]; then
            echo -e "Rotating and Scaling $img, saving to $output" 
            convert $img -rotate -90 ${output}
            # If you want to cut the image to adapt it to the size
            convert ${output} -resize "${size}^" -gravity center -extent ${size} -quality ${quality} ${output}
            
            # If you are oki with white stripes in the eInk display
            # convert ${output} -resize ${size} ${output}
        else
            echo -e "Scaling $img, saving to $output"
            convert $img -resize "${size}^" -gravity center -extent ${size} -quality ${quality} ${output}
            # convert $img -resize 960x540 ${output}
        fi
        mv $img "originals/$img"
    fi
done
