# Bot Stockpile - planner bot

ChatScript comes with a simple planner bot called Stockpile. You build it using `:build stockpile`.

The world of stockpile has three locations: location0, location1, and location2. It has two kinds of
resources: stone and wood. And it has three kinds of transport: carts, trains, and ships. Each mode of
transport can carry a different amount of material and has different requirements on connectivity. In the
prebuilt demo, location0 is an island and location1 and location2 are on land adjacent to each other and
both coastal. There is no railroad line, so trains are useless. Each location has an initial amount of
some resource.

You get to tell the system how much of a resource you want somewhere and it's job is to try to move
things around to achieve that (not necessarily most efficiently). At each moment it tells you how much
of what is where.

You can say this to get the system to do some work: `pile location1 wood 3` (the command is pile, with arguments location, material, quantity). Or this to add material to the world: `place location1 stone 3`. Or you can ask the world to describe its current state with this (although it generally tells you anyway): `show`
