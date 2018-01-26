@echo off

php "..\builder\make_resource.php" ".\src\resource.hpp"
php "..\builder\make_locale.php" "Free Shooter" "freeshooter" ".\bin\i18n" ".\src\resource.hpp" ".\bin\freeshooter.lng"
copy /y ".\bin\freeshooter.lng" ".\bin\32\freeshooter.lng"
copy /y ".\bin\freeshooter.lng" ".\bin\64\freeshooter.lng"

pause
