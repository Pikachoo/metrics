bool a= false;

label:
for(int i = 0; i< 10; i++)
{

   if(a==true)
    {
        i--;
        goto label;
    }
    else
    {

        i++;
        a = true;
    }

}

