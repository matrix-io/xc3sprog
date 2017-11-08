{{#success build.status}}
  {{build.author}} just built `{{repo.name}}:{{build.branch}}` from <%%DRONE_COMMIT_LINK%%|#{{truncate build.commit 8}}>
  :new: {{build.message}}
  :debian: The new `matrixio-xc3sprog_%%DISTRIBUTION%%-%%CODENAME%%-%%PKG_VER%%-%%COMPONENT%%_armhf.deb` was published to apt.matrix.one/%%DISTRIBUTION%% %%CODENAME%% %%COMPONENT%%
{{else}}
  {{build.author}} just broke the build of `{{repo.name}}:{{build.branch}}` with <%%DRONE_COMMIT_LINK%%|#{{truncate build.commit 8}}>
  :new: :zombie: {{build.message}}
  :debian: The package `matrixio-xc3sprog_%%DISTRIBUTION%%-%%CODENAME%%-%%PKG_VER%%-%%COMPONENT%%_armhf.deb` failed to build.
{{/success}}
:stopwatch: {{ since build.started}}
:gear: {{build.link}}
