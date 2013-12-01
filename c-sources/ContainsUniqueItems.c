static int
wl_iw_get_country(
        struct net_device *dev,
        struct iw_request_info *info,
        union iwreq_data *wrqu,
        char *extra
)
{
	char *ccode;
	int current_channels;

	WL_TRACE(("%s\n", __FUNCTION__));

	ccode = dhd_bus_country_get(dev);
	if(ccode){
		if(0 == strcmp(ccode, "Q2"))
			current_channels = 11;
		else if(0 == strcmp(ccode, "EU"))
			current_channels = 13;
		else if(0 == strcmp(ccode, "JP"))
			current_channels = 14;
	}
	sprintf(extra, "Scan-Channels = %d", current_channels);
	printk("Get Channels return %d,(country code = %s)\n",current_channels, ccode);
	return 0;
}
