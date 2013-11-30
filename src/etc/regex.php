<?

namespace rude;

class regex
{
	public static function erase_comments($string)
	{
		$matches = regex::comments($string);

		return str_replace($matches, '', $string);
	}

	public static function erase_strings($string)
	{
		$matches = regex::strings($string);

		return str_replace($matches, '', $string);
	}

	public static function comments(&$string)
	{
		return array_merge(regex::comments_slashstar($string), regex::comments_slashslash($string));
	}

	public static function strings(&$string)
	{
		return array_merge(regex::strings_quote($string), regex::strings_quotequote($string));
	}

	public static function comments_slashstar(&$string)
	{
		if (preg_match_all('#/\*(.*?)\*/#sm', $string, $matches_slashstar))
		{
			return $matches_slashstar[0];
		}

		return array();
	}

	public static function strings_quote(&$string)
	{
		if (preg_match_all('/\'(.*?)\'/sm', $string, $matches_strings))
		{
			return $matches_strings[0];
		}

		return array();
	}

	public static function strings_quotequote(&$string)
	{
		if (preg_match_all('/"(.*?)"/sm', $string, $matches_strings))
		{
			return $matches_strings[0];
		}

		return array();
	}


	public static function comments_slashslash(&$string)
	{
		if (preg_match_all('#//(.*)$#m', $string, $matches_slashslash))
		{

			return $matches_slashslash[0];
		}

		return array();
	}

    public  static function  get_goto_label(&$string)
    {
        if (preg_match_all('/(?<=goto )(.*?)(?=;)/sm', $string, $matches_goto))
        {
            ?><pre><?print_r($matches_goto[0])?></pre><?
            return $matches_goto[0];
        }

        return array();

    }
}