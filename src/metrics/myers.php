<?

namespace rude;

require_once (getcwd() . DIRECTORY_SEPARATOR . 'src' . DIRECTORY_SEPARATOR . 'etc' . DIRECTORY_SEPARATOR . 'lexer.php');

class myers
{
	private $file_data;
	private $file_count;

	private $lexer;

	private $ZG;  // число уникальных операторов программы, включая символы-разделители, имена процедур и знаки операций (словарь операторов)
	private $h;

	public function __construct($file_data, $file_count)
	{
		$this->file_data = $file_data;
		$this->file_count = $file_count;

		$this->file_data = regex::erase_comments($this->file_data);
		$this->file_data = regex::erase_strings($this->file_data);

        $this->h = $this->count_predicate();

		$this->lexer = new lexer($this->file_data);
	}

	public function get_metrics()
	{



		$ev  = $this->lexer->count_conditions()+$this->lexer->count_loops()+$this->count_operators_GOTO();
		$p  = $this->lexer->count_returns();

        $this->ZG = $ev + 2*$p;
        $this->h = $this->count_predicate();




		$metrics = array();


		$metrics[] = array('Z(G)', $this->ZG);
		$metrics[] = array('h',    $this->h,);
		$metrics[] = array('[Z(G),Z(G)+h]',   '['.$this->ZG.','.($this->ZG+$this->h).']');

		return $metrics;



		return $metrics;
	}

    public function  get_positions($list_of_name)
    {

        $list_of_name = array_unique($list_of_name);
        $list = array();
        foreach($list_of_name as $name)
        {
            $name_positions_list = $this->find_all_positions($this->file_data,$name);
            foreach($name_positions_list as $position)
            {
                $array = array();

                $array['name'] = $name;
                $array['position'] = $position;

                $list[] = $array;
            }
        }
        return $list;
    }

    public function find_all_positions(&$string, &$find)
    {
        $offset = 0;

        $match = array();

        for ($count = 0; (($pos = strpos($string, $find, $offset)) !== false); $count++)
        {
            $match[] = $pos;
            $offset = $pos + strlen($find);
        }





        return $match;
    }

    public function get_label_and_goto_lists()
   {
        $goto_list = regex::get_goto_whith_label($this->file_data);
        $goto_list = $this->get_positions($goto_list); //получаем в goto_list именнованный массив, [name] = goto label, [position] = pos

        $label_list = array();


        foreach ($goto_list as &$goto)
        {
            $words = regex::get_words_from_string($goto['name']);



            if (!isset($words[1])) { continue; }
            $goto['name'] = $words[1];

            $labels = regex::get_label_list($this->file_data, $words[1]);
            $temp = $this->get_positions($labels);
            if(!empty($temp[0]))
                $item = $temp[0];
            $item['name'] = trim($item['name'],":");

            $label_list[] = array('name' => $item['name'], 'position' => $item['position']);
        }

//       ?><!--<pre>--><?// $label['position'] ?><!--</pre>--><?//

        return array('goto_list' => $goto_list, 'label_list' => $label_list);
   }

public  function check_between_goto_and_label($goto,$label)
{
        $position = $goto['position'];
        while($this->file_data[$position] != ';')
        {
            $position++;
        }

        $position++;


        while($label['position'] != $position)
        {

//            ?><!--<pre>--><?// print_r($this->file_data[$position]) ?><!--</pre>--><?////
//
            if(isset(regex::check_character($this->file_data[$position])[0]) == true)
            {
                echo "lalala";
                return true;
            }
            $position++;

        }
        return false;

}
	public function count_operators_GOTO()
	{

        $array = $this->get_label_and_goto_lists();

        $count = 0;

//        ?><!--<pre>--><?// $label['position'] ?><!--</pre>--><?//
        foreach ($array['goto_list'] as $goto)
        {

            foreach($array['label_list'] as $label)
            {
                if($goto['name'] == $label['name'])
                {

                    if($this->check_between_goto_and_label($goto,$label) == false)
                        continue;
                    if($goto['position'] < $label['position'])
                    {
                        $count += 2;
                    }
                    else if($goto['position'] > $label['position'])
                    {
                        $count++;
                    }

                }
            }

        }




        return $count;

	}

    public  function  get_positions_of_conditions($begin)
    {
        $position_list = $this->find_all_positions($this->file_data,$begin);

        foreach($position_list as $key => $position )
        {
            $temp = regex::check_character($this->file_data[$position-1]);
            $temp1 = regex::check_character($this->file_data[$position+2]);
            if ((!empty($temp[0]) == true )|| (!empty($temp1[0]) == true))
            {
                unset($position_list[$key]);
            }

        }
//        ?><!--<pre>--><?// print_r($position_list) ?><!--</pre>--><?//
        return $position_list;

    }



    public function parse_between($begin)
    {

        $position_list = $this->get_positions_of_conditions($begin);

        $predicates_list = array();

        foreach ($position_list as $position)
        {
            $position += 2;
            $character = $this->file_data[$position];
            $opening_bracket = 0;
            $closing_bracket = 0;
            $predicate = "";

            while(true)
            {
                if($character == '(')
                {
                    $opening_bracket++;
                    $predicate .= $character;
                    $position++;
                    $character = $this->file_data[$position];
                    break;
                }
                $position++;
                if(isset($this->file_data[$position]))
                    $character = $this->file_data[$position];

            }


            while(true)
            {

                if($character == '(')
                {
                    $opening_bracket++;
                }

                if($character == ')')
                {
                    $closing_bracket++;
                }

                $predicate .= $character;
                $position++;

                if(isset($this->file_data[$position]))
                {
                    $character = $this->file_data[$position];
                }
                else
                    break;


                if($closing_bracket == $opening_bracket)
                    break;

            }
            $predicates_list[] = $predicate;
        }
//
//        ?><!--<pre>--><?// print_r($predicates_list) ?><!--</pre>--><?//

return $predicates_list;

}

public function source_count($item, $source, $odd_list = array())
{
    $count = substr_count($source, $item);
    foreach ($odd_list as $odd)
    {
        $count -= substr_count($source, $odd);
    }
    return $count;
}

public function count_operators($source)
{
    $total_operators = 0;

    $operators = array();

    $operators[] = array('<',   $this->source_count('<',  $source, array( '<=')));
    $operators[] = array('>',   $this->source_count('>',  $source, array( '>=')));


    $operators[] = array('>=',  $this->source_count('>=', $source));
    $operators[] = array('<=',  $this->source_count('<=', $source));
    $operators[] = array('==',  $this->source_count('==', $source));
    $operators[] = array('!=',  $this->source_count('!=', $source));

    sort($operators);


    foreach ($operators as $operator)
    {
        if (isset($operator[1]))
        {
            $total_operators += intval($operator[1]);
        }
    }

    return $total_operators;
}

public  function  count_predicate()
{

    $predicate_list = $this->parse_between("if");
    $h = 0;
    foreach($predicate_list as $predicate)
    {
        $count = $this->count_operators($predicate);
        if($count > 1)
        {

            $count--;
            $h += $count;
        }

    }

    $predicate_list = $this->parse_between("while");
    foreach($predicate_list as $predicate)
    {
        $count = $this->count_operators($predicate);
        if($count > 1)
        {
            $count--;
            $h += $count;
        }

    }


    return $h;

}



}