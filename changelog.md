## v1.0.8

- Added **Discord Linking** in the gauntlet layer

## v1.0.7

- Added **Likes and Dislikes** on Gauntlets. You can give a like or dislike to a gauntlet, and the total likes and dislikes will be displayed on the gauntlet info popup.
- Fixed an issue with local gauntlet levels where beating the same level from an online gauntlet was also counted as beating the local gauntlet level.
- Fixed random crashes related to cached image destruction (CCTextureAtlas dtor) on background threads.
- Fixed issue where saving a gauntlet with a custom sprite will not save the sprite if the original sprite is deleted on the device.

## v1.0.6

- Added **Tags Display** on the Gauntlet list. You can click on the tags to view the full list of tags in a popup.
- Added **Tags** on the Credits Popup.
- Added **Featured Gauntlet** in the Local Gauntlet list.
- Fixed a bug where syncing your online gauntlet progression would reset your local gauntlet progression.
- Fixed a bug where tags with spaces didn't display properly on the gauntlet list.
- Adjusted tag cell sizes to fit the UI better.
- Nav arrows are hidden when there is only a single page of gauntlets.

## v1.0.5

- Added **Local Gauntlets**: Users can create their own gauntlet lists.
  _(This does not reward points and does not sync with the backend; it is purely client-side and for fun)_
- Gauntlets will now loop back when navigating next from the last gauntlet or previous from the first gauntlet.
- Added **Search Gauntlets** feature, allowing users to search for gauntlets by their name.
- Added **Skull Icon** on the last gauntlet level.
- Added **Tag System** for online gauntlets.
- Added gauntlet reward points on the level info when opening from the gauntlet levels selection.
- Tweaked the UI for the Gauntlet Manager Popup.
- Replaced the Background Index text input with a button to select backgrounds for improved user-friendliness.
- Adjusted the completed icon position to match the vanilla gauntlet completed icon.
- Fixed a bug where completing a level in practice mode would trigger gauntlet completion.

## v1.0.4

- Added **Featured Gauntlet**, which you can identify by the glowing particle effect.
- Added **Recent Gauntlet** toggle.
- Tweaked the Add/Edit Gauntlet Popup UI.
- Added a new **Gauntlet Credits** section in the Gauntlet Levels Selection.
- Added custom backgrounds for gauntlets in the Levels Selection.
- Added additional safeguards to prevent players from receiving rewards outside of gauntlet levels.
- Fixed the description input field in the Gauntlet Manager not typing correctly.
- Fixed level completions not registering properly.
- Fixed badges duplicating on the profile page.
- Fixed a backend issue relating to contributor status.
- Fixed typos.

## v1.0.3

- Gauntlets are locked until you complete the previous one. This is to prevent skipping levels and encourage players to complete the gauntlets in order.
- Improved the **Anti-Cheat** system.
- Fixed a bug where an outdated gauntlet sprite would still be shown.

## v1.0.2

- Added a **Discord Button** on the main menu.
- Added **Badge Descriptions** when clicking on badges on the profile page or comment cells.
- Added **Promote User** feature for Gauntlet Managers.
- Improved the **Gauntlet Info Popup** with more information about the mod and its features.
- Fixed **Gauntlet Role Badge** scaling on comment cells.
- The completed mark on Gauntlet Levels is now applied when you finish a level.

## v1.0.1

- Massive blunder. Fixed a crash when clicking the sync button on mobile.
- Fixed the description on the mod page.

## v1.0.0

- Gauntlet Deluxe is good! Better than Rated Layouts?
