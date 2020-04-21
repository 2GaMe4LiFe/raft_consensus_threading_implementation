# HANSI latex setup

sudo apm install latex
sudo apm install language-latex
sudo apm install pdf-view

brew cask install basictex # Running Homebrew as root is extremely dangerous
sudo tlmgr update --self
sudo tlmgr install latexmk
sudo tlmgr install subfiles

latexmk --version
