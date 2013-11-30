<?

namespace rude;


class html
{
	public static function table($rows, $cols)
	{
		?>
		<table>
			<tbody>
			<tr>
				<?
					foreach ($cols as $col)
					{
						?><td><b><?= $col ?></b></td><?
					}
				?>
			</tr>
			<?
				foreach ($rows as $row)
				{
					?>
					<tr>
						<?
							foreach ($row as $item)
							{
								?><td><?= $item ?></td><?
							}
						?>
					</tr>
				<?
				}
			?>
			</tbody>
		</table>
		<?
	}

	public static function table_horizontal($cols, $rows)
	{
		?>
		<table>
			<tbody>
			<tr>
				<td>
					<b><?= $rows[0] ?></b>
				</td>
				<?
					foreach ($cols as $col)
					{
						?><td><?= $col[0] ?></td><?
					}
				?>
			</tr>
			<tr>
				<td>
					<b><?= $rows[1] ?></b>
				</td>
				<?
					foreach ($cols as $col)
					{
						?><td><?= $col[1] ?></td><?
					}
				?>
			</tr>
			</tbody>
		</table>
	<?
	}
} 