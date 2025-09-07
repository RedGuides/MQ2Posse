---
tags:
  - plugin
resource_link: "https://www.redguides.com/community/resources/mq2posse.156/"
support_link: "https://www.redguides.com/community/threads/mq2posse.63369/"
repository: "https://github.com/RedGuides/MQ2Posse"
config: "MQ2Posse.ini"
authors: "eqmule, CTaylor22, sym, plure"
tagline: "Checks for PCs in a defined radius identifying them as friends or strangers"
---

# MQ2Posse

<!--desc-start-->
MQ2Posse allows you to create a friends list so any PC coming into range will be identified as a friend or stranger. You can also set up alerts and reactions to these events.

* If a stranger is identified you can run a custom command, such as `/endmacro` and `/mac GTFO.mac`
* It also has a TLO that allows you to use MQ2Posse in your own scripts.
<!--desc-end-->

## Commands

<a href="cmd-posse/">
{% 
  include-markdown "projects/mq2posse/cmd-posse.md" 
  start="<!--cmd-syntax-start-->" 
  end="<!--cmd-syntax-end-->" 
%}
</a>
:    {% include-markdown "projects/mq2posse/cmd-posse.md" 
        start="<!--cmd-desc-start-->" 
        end="<!--cmd-desc-end-->" 
        trailing-newlines=false 
     %} {{ readMore('projects/mq2posse/cmd-posse.md') }}

## Settings

An example MQ2Posse.ini file,  
The global name list must be manually entered into the MQ2Posse.ini file, like so:

```ini
[GlobalNames]
galndalf=1
grimli=1
bildo=1

[Bob_Settings]
Enabled=1
Radius=300
Soundfile=
Audio=1
IgnoreGuild=1
Notify=1

[Bob_Names]
bernard=1
alexander=1
corina=1

[Bob_Commands]
0=/endmacro
1=/say gotta go, bye bye!

[Tim_Settings]
Enabled=1
Radius=300
Soundfile=
Audio=1
IgnoreGuild=1
Notify=1

[Tim_Names]

[Tim_Commands]
0=/endmac
1=/mac GTFO
```

## Top-Level Objects

## [Posse](tlo-posse.md)
{% include-markdown "projects/mq2posse/tlo-posse.md" start="<!--tlo-desc-start-->" end="<!--tlo-desc-end-->" trailing-newlines=false %} {{ readMore('projects/mq2posse/tlo-posse.md') }}

## DataTypes

## [Posse](datatype-posse.md)
{% include-markdown "projects/mq2posse/datatype-posse.md" start="<!--dt-desc-start-->" end="<!--dt-desc-end-->" trailing-newlines=false %} {{ readMore('projects/mq2posse/datatype-posse.md') }}

<h2>Members</h2>
{% include-markdown "projects/mq2posse/datatype-posse.md" start="<!--dt-members-start-->" end="<!--dt-members-end-->" %}
{% include-markdown "projects/mq2posse/datatype-posse.md" start="<!--dt-linkrefs-start-->" end="<!--dt-linkrefs-end-->" %}
