
welcomeSetup()
{
    echo "Setting up Homepage"
    readPrefType
    read -p "New URL: " welcomeURL
    echo "Setting $prefString Homepage to $welcomeURL"
    kdb setmeta "${MountPoint}/${prefType}/browser/startup/homepage" type string
    kdb set "${MountPoint}/${prefType}/browser/startup/homepage" "$welcomeURL"
}

welcomeSetup
