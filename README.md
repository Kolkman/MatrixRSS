# Matrix RSS MQTT version

This is a matrix led display that shows time, temperature, and latest news.

Temperature and news items are obtained from MQTT (rss/news).

I am using Feedreader plugin with homneassistant and an automation that publishes the title to mqtt (follow the example [here](https://www.home-assistant.io/integrations/feedreader/). 



Another [branch (RSS-Internal)](https://github.com/Kolkman/MatrixRSS/tree/RSS-Internal) has a build in news reader - but that is less stable, probably because of heap fragmentation.
