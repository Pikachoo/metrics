<?

namespace rude;

class arrays
{
	public static function combine(&$array_first, &$array_second)
	{
		$result = array();

		if (!empty($array_first) &&  empty($array_second)) { return $array_first;  }
		if ( empty($array_first) && !empty($array_second)) { return $array_second; }


		foreach ($array_first as $item_first)
		{
			if (!isset($item_first[0]) || !isset($array_first[1])) { continue; }


			foreach ($array_second as $item_second)
			{
				if (!isset($item_second[0]) || !isset($item_second[1])) { continue; }


				if ($item_first[0] == $item_second[0])
				{
					$item = array($item_first[0], $item_first[1] + $item_second[1]);

					     if (isset($item_first[2]))  { $item[] = $item_first[2];  }
					else if (isset($item_second[2])) { $item[] = $item_second[2]; }

					$result[] = $item;
				}
			}
		}

		return $result;
	}
}