<?

namespace rude;

class string
{
	public static function is_exists($substring, $string)
	{
		if (strrpos($string, $substring) !== false)
		{
			return true;
		}

		return false;
	}

	public static function explode($string, $substring)
	{
		return explode($substring, $string);
	}
}