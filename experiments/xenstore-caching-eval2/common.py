default_confile = 'config.xl'
default_prefix = 'mini_os_'
name_attr = 'name = "'


def replace_confile(config, newnum, config_file = default_confile, domname_prefix = default_prefix):
    if (not config):
        with open(config_file, 'r') as confile:
            config = confile.read()

    currname = config.split(name_attr)[1].split('"')[0]
    newname = domname_prefix + str(newnum)

    if not currname:
        pos = config.find(name_attr)
        pos += len(name_attr)
        config = config[0:pos] + newname + config[pos:]

    else:
        config = config.replace(currname, newname)

    with open(config_file, 'w') as confile:
        confile.write(config)


#replace_confile(None, 29)
