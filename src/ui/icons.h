#ifndef ICONS_H
#define ICONS_H

class QIcon;

class Icons
{
public:
    static char Back[];
    static char Forward[];
    static char OpenLink[];
    static char Plus[];
    static char Minus[];
    static char CloseCircled[];
    static char CloseRound[];
    static char Close[];
    static char Search[];
    static char DocumentText[];
    static char WorldOutline[];

    static QIcon getIcon(char text[]);
};

#endif // ICONS_H
