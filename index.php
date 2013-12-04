<?

require_once 'src/rude-metrics.php';

$filesystem = new rude\filesystem;

$file_list = $filesystem->scandir(__DIR__ . DIRECTORY_SEPARATOR . 'c-sources', 'c');
$file_data = '';


foreach ($file_list as $file_path)
{
	$file_data .= $filesystem->read($file_path) . PHP_EOL;
}


$chapin_metrics = array();

foreach ($file_list as $file_path)
{
	$chapin = new rude\chapin($filesystem->read($file_path));;

	$chapin_metrics = rude\arrays::combine($chapin_metrics, $chapin->get_metrics());
}



//$chapin   = new rude\chapin($file_data);
$halstead = new rude\halstead($file_data, count($file_list));
$myers    = new rude\myers($file_data, count($file_list));

?>
<html>
	<head>
		<link href="style.css" rel="stylesheet" type="text/css" />
	</head>

	<body>
        <div id = "main">
            <table >
                <td>
                <div id="container">
                    <h2>Файл</h2>

                    <label for="source"></label>
                    <textarea id="source"><?= htmlentities(\rude\filesystem::read('c-sources'.DIRECTORY_SEPARATOR.'source.c')); ?></textarea>
<!--                    <textarea id="source">--><?// foreach ($file_list as $file_path) { echo $file_path . PHP_EOL; } ?><!--</textarea>-->
                </td>
                <td>
                    <div id = "metrics">

                        <div id="halstead">
                            <a>Метрика Холстеда</a>
                            <? rude\html::table_horizontal($halstead->get_metrics(),array()) ?>
                        </div>

                        <div id="chapin">
                            <a>Метрика Чепина</a>
                            <? rude\html::table_horizontal($chapin_metrics) ?>
                        </div>

                        <div id="chapin">
                            <a>Метрика Майерса</a>
                            <? rude\html::table_horizontal($myers->get_metrics()) ?>
                        </div>
                    </div>
                </td>

                </div>
            </table>
    </div>
	</body>
</html>