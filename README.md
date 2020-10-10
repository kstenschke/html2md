html2md
=======

## Table of Contents

- [What does it do?](#what-does-it-do?)
- [Usage Example](#usage-example) 
- [Code Convention](#code-convention)
- [License](#license)


## What does it do?

html2md is a C++ single header solution for HTML to markdown conversion.  


### Usage Example

````
#include <html2md.hpp>

int main() {
    std::cout << html2md::Html2Text(html);
}
````
See the included example.cpp 


Code Convention
---------------

The source code of the html2md header follows the 
[Google C++ Style Guide](https://google.github.io/styleguide/cppguide.html).  


License
-------

html2md is licensed under
[The MIT License (MIT)](https://opensource.org/licenses/MIT)
