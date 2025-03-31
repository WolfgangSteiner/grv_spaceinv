#!/usr/bin/env fish

function spaces_to_tabs
    for file in $argv
        echo $file
        unexpand --tabs=4 $file >$file.unexpand
        mv $file.unexpand $file
    end

end

spaces_to_tabs $argv
