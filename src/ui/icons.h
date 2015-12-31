#ifndef ICONS_H
#define ICONS_H

class QIcon;

class Icons
{
public:
    static char CloseCircled[];
    static char CloseRound[];
    static char Close[];
    static char Search[];
    static char DocumentText[];
    static char WorldOutline[];

    static QIcon getIcon(char text[]);
};

#endif // ICONS_H
