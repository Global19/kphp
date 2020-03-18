@ok
<?php
require_once 'polyfills.php';
require_once 'Classes/autoload.php';

function demo1Instance() {
    $t = shape(['n' => 1, 's' => 'str', 'a' => new Classes\A]);

    $int = $t['n'];
    echo $int, "\n";

    /** @var Classes\A */
    $a = $t['a'];
    $a->printA();
}

function get2Instances() {
    return shape(['n' => 1, 's' => 'str', 'a' => new Classes\A, 'b' => new Classes\B]);
}

function demo2Instances() {
    $t = get2Instances();

    $int = $t['n'];
    echo $int, "\n";

    /** @var Classes\A */
    $a = $t['a'];
    $a->printA();

    /** @var Classes\B */
    $b = $t['b'];
    echo $b->setB1(5)->b1, "\n";
}

demo1Instance();
demo2Instances();