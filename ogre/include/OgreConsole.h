#ifndef OGRECONOSLE_H
#define OGRECONOSLE_H

#include "OgreConsolePreRequisite.h"
#include "pacConsole.h"

namespace pac {

class _PacExport OgreConsole : public Console {
public:
  OgreConsole(ConsoleUI* ui, Ogre::SceneManager* sceneMgr);

  virtual bool execute(const std::string& cmdLine = "");
  /**
   * The scene need better be freezed when you want to change properties of some
   * movable object  or scenenode, otherwise if the editting object got removed
   * from scene, your next set type command will crash.
   */
  virtual void freeze() { this->outputLine("freezing....."); }
  /**
   * restore back scene
   */
  virtual void melt() { this->outputLine("melting....."); }
  virtual bool isFrozen() { return false; }

  Ogre::SceneManager* getSceneMgr() const { return mSceneMgr; }
  void setSceneMgr(Ogre::SceneManager* v) { mSceneMgr = v; }

  AbsDir* getMovableDir() const { return mMovableDir; }
  AbsDir* getNodeDir() const { return mNodeDir; }
  AbsDir* getSceneDir() const { return mSceneDir; }

protected:
  /**
   * registered with following arg handler
   *
   * movable:
   *   particle
   *   light
   *   camara
   *   entity
   * resource:
   *   material
   *   mesh
   *   texture
   *   compositor
   *   pst    (particle system template)
   * node:
   *   bone
   *   sceneNode
   * enum:
   *   en_lightType
   *   en_polygonMode
   *   en_fogMode
   *   en_cullingMode
   * string:
   *   moType   (movable type)
   *   affector_force_application
   * literal:
   *   all
   *   ancestor
   *   camera
   *   direct
   *   entity
   *   light
   *   parent
   *   particle
   *   sceneNode
   *   tagPoint
   */
  virtual void initArghandler();

  // just some place to shrink initArgHandler
  virtual void initCommand();
  virtual void initDir();
  virtual void initEnumArgHandler();
  virtual void initResourceArghandler();
  virtual void initMiscArgHandler();
  virtual void initStringArgHandler();
  virtual void initLiteralArgHandler();

protected:
  Ogre::SceneManager* mSceneMgr;
  AbsDir* mMovableDir;
  AbsDir* mNodeDir;
  AbsDir* mSceneDir;
};
}

#endif /* OGRECONOSLE_H */
