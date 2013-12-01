<?

require_once 'src/rude-metrics.php';

$filesystem = new rude\filesystem;

$file_list = $filesystem->scandir(__DIR__ . DIRECTORY_SEPARATOR . 'java-sources', 'java');
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
$jilb     = new rude\jilb($file_data);
$myers    = new rude\myers($file_data, count($file_list));

?>
<html>
	<head>
		<link href="style.css" rel="stylesheet" type="text/css" />
	</head>

	<body>
		<div id="container">
			<h2>Список файлов</h2>

			<label for="source"></label>
			<textarea id="source"><? foreach ($file_list as $file_path) { echo $file_path . PHP_EOL; } ?></textarea>

			<div id="dictionary">
				<? rude\html::table_horizontal($jilb->get_dictionary(), array('Токен', 'Найдено')) ?>
			</div>

			<div id="halstead">
				<? rude\html::table($halstead->get_metrics(), array('Метрика', 'Значение', 'Описание')) ?>
			</div>

			<div id="chapin">
				<? rude\html::table($chapin_metrics, array('Метрика', 'Значение', 'Описание')) ?>
			</div>

			<div id="total">
				<? rude\html::table($jilb->get_loops(),      array('Операнд', 'Найдено')) ?>
				<? rude\html::table($jilb->get_conditions(), array('Операнд', 'Найдено')) ?>
			</div>

			<div id="metrics">
				<? rude\html::table($jilb->get_metrics(), array('Метрика', 'Значение', 'Описание')) ?>
			</div>
		</div>
	</body>
</html>