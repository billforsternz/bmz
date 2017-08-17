Introduction to BMZ
===================

This is BMZ, a very simple embedded operating system and framework. The
key idea of BMZ is that it provides not just OS style primitives
(timers, tasks messaging etc.) but a framework for your embedded code.
This means that to build your system you don't start from scratch with
main(). Instead you start by effectively creating a block diagram in
code. You name the blocks and the connections between them. BMZ provides
the tasks to implement the blocks and the messages and queues to
implement the connections. All you have to do is implement a group of
up to five handlers per task. An init handler, a timeout handler, an
idle time handler and message handlers to process received messages from
up to two sources. All five are optional.

BMZ was created for Zilog 2004 Flash Nets Cash design competition run by
Circuit Cellar magazine. I did receive a small prize and an honourable
mention. The competition was really targetted at projects with a
hardware component, and sadly as a 100% software guy (basically) I was
limited to building something that just used the hardware on the
provided development board. So the first BMZ project was a simple
terminal server that served to make reasonable use of an ethernet port
and two serial ports. It was a working project though and did require me
to build a simple TCP/IP stack in BMZ.

BMZ stands for "Bare Metal Zilog" but nothing about it is really tied to
the Zilog processor and hardware it originally ran on.

I decided to put BMZ up on Github to prevent it becoming lost forever.
It is now necessary to use the Internet Archive to find anything of the
original competition and project entry. I think the project has merit,
the embedded framework concept is probably not novel, but I haven't ever
seen anything exactly like it. I made use of BMZ on a few subsequent
projects, and I would use it again without hesitation for the right kind
of project. What is the right kind of project? Basically a simple bare
metal embedded application on a small resource constrained
microprocessor or microcomputer. Something with maybe a communications
port of some kind, a few switches, LEDs, maybe a display. The sort of
thing I made a living from back in the day.

I am starting today with the original, unaltered competition entry. I
will tag that with Git as V1.00. I will endeavour to dig out some
subsequent fixes and improvements from later projects.

What I would really love to do is use BMZ as the basis of scriptable
simulation system that would enable you to develop and test your
embedded code to the maximum extent possible on your desktop computer.
That was an approach that was very successful for me back in the day,
but all the code involved was proprietary to the companies I was working
for at the time. I'd love to revive the ideas though, and BMZ could be
an important building block in a complete system.

Bill Forster <billforsternz@gmail.com> Last update: 17Aug2017
