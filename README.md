# syspro3
A project consisting of the following executables: 

1. A **Bash** script that creates all the webpages of a simple, text-only website in a predefined manner so that they link to each other in a desired way.   
1. A web server that connects to a requested port and responds to HTTP/1.1 GET requests made to the webpages created from _(1)_.  
1. A web crawler that communicates with the server in _(2)_ and starting from one of these webpages attempts to download the entirety of the website and save it locally. It can also simultaneously receive commands from another port to perform word searches in the downloaded website by executing [jobExecutor](https://github.com/TsiarasKon/jobExecutor).  

Both _(2)_ and _(3)_ were created entirely in **C99** and make use of socket programming and multi-threading.

